using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace Biometrics
{
    public class FingerPrintHelperLirox : IDisposable, IFingerPrintHelper
    {
        public event EventHandler<FingerPrintEventArgs> Notification;

        private CancellationTokenSource tokenSource;
        private CancellationToken token;

        private static SemaphoreSlim semaphore;

        public bool IsConnected { get; private set; }
        public string StorePath { get; set; }
        public string ImportPath { get; set; }
        public string FileNameModel { get; set; } = "Template-{0}.lr.fpt";

        private const string searchPattern = "*.lr.fpt";

        private readonly Dictionary<int, byte[]> store = new Dictionary<int, byte[]>();

        private bool IsTimeToDie = false;
        private bool IsRegister = false;
        private bool IsIdentify = false;

        readonly byte[] refbuf = new byte[512];

        private int matsize = 0;
        private readonly byte[] matbuf = new byte[256];

        private int refsize = 0;
        private int FingerPrintId = 1;

        private readonly Regex extractIdFromFileNamePattern = new Regex(@"Template-(\d+).lr.fpt");

        public FingerPrintHelperLirox()
        {
            tokenSource = new CancellationTokenSource();
            token = tokenSource.Token;

            semaphore = new SemaphoreSlim(1);
        }

        protected virtual void RaiseNotification(FingerPrintEventArgs e)
        {
            Notification?.Invoke(this, e);
        }

        private void refreshToken()
        {
            if (token.IsCancellationRequested)
            {
                tokenSource = new CancellationTokenSource();
                token = tokenSource.Token;
            }
        }

        public void CancelOperation()
        {
            if (tokenSource != null)
                tokenSource.Cancel();
        }

        public Task Start(string deviceConnection = "USB")
        {
            try
            {
                fpengine.CloseDevice();
                if (fpengine.OpenDevice(0, 0, 0) == 1)
                {
                    if (fpengine.LinkDevice(0) == 1)
                    {
                        Thread captureThread = new Thread(new ThreadStart(DoCapture))
                        {
                            IsBackground = true
                        };

                        Restore();

                        captureThread.Start();

                        IsTimeToDie = false;

                        Identify();

                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionSuccess));
                    }
                    else
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                        return Task.CompletedTask;
                    }
                }
                else
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                    return Task.CompletedTask;
                }
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail, ex.GetBaseException().Message));
            }

            return Task.CompletedTask;
        }

        private void DoCapture()
        {
            while (!IsTimeToDie)
            {
                int wm = fpengine.GetWorkMsg();
                int rm = fpengine.GetRetMsg();

                switch (wm)
                {
                    case fpengine.FPM_DEVICE:
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, "Not Open Device"));
                        break;
                    case fpengine.FPM_PLACE:
                        if (!IsIdentify)
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
                        else
                            Debug.WriteLine("Put finger to identify");
                        break;
                    case fpengine.FPM_LIFT:
                        if (!IsIdentify)
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.RaiseFinger));
                        else
                            Debug.WriteLine("Raise finger to identify");
                        break;
                    case fpengine.FPM_CAPTURE:
                        break;
                    case fpengine.FPM_ENROLL:
                        {
                            try
                            {
                                if (rm == 1)
                                {
                                    fpengine.GetTemplateByEnl(refbuf, ref refsize);

                                    var bytesTosave = new byte[refsize];
                                    Array.Copy(refbuf, bytesTosave, refsize);

                                    int fid = StoreIdentify(bytesTosave);

                                    if (fid >= 0)
                                    {
                                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                                    }
                                    else
                                    {
                                        store.Add(FingerPrintId, bytesTosave);

                                        SaveTemplate(bytesTosave, FingerPrintId);

                                        FingerPrintId++;
                                    }
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, "Enroll fail"));
                                }
                            }
                            catch (Exception ex)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ImportError, ex.GetBaseException().Message));
                            }

                            IsRegister = false;
                            IsIdentify = true;

                            fpengine.CaptureTemplate();
                        }
                        break;
                    case fpengine.FPM_GENCHAR:
                        {
                            try
                            {
                                if (rm == 1)
                                {
                                    fpengine.GetTemplateByCap(matbuf, ref matsize);

                                    int fid = StoreIdentify(matbuf);

                                    if (fid >= 0)
                                    {
                                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifySuccess, fid));
                                    }
                                    else
                                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifyFail));
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, "Capture Fail"));
                                }
                            }
                            catch (Exception ex)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ImportError, ex.GetBaseException().Message));
                            }

                            fpengine.CaptureTemplate();
                        }
                        break;
                    case fpengine.FPM_NEWIMAGE:
                        break;
                    case fpengine.FPM_TIMEOUT:
                        Debug.WriteLine("Timeout");
                        if (IsRegister)
                        {
                            fpengine.EnrollTemplate();
                            IsRegister = false;
                            IsIdentify = false;
                            fpengine.SetTimeOut(10);
                        }
                        else
                        {
                            IsIdentify = true;
                            fpengine.CaptureTemplate();
                        }
                        break;
                }

                Thread.Sleep(200);

                Import(true);
            }
        }

        public Task Store()
        {
            IsRegister = true;

            fpengine.SetTimeOut(0);

            return Task.CompletedTask;
        }

        public Task Identify()
        {
            IsRegister = false;
            IsIdentify = true;

            fpengine.CaptureTemplate();

            return Task.CompletedTask;
        }

        public async Task DelFingerPrint(int id)
        {
            try
            {
                await semaphore.WaitAsync();

                try
                {
                    string filename = $@"{StorePath}\{string.Format(FileNameModel, id)}";

                    if (File.Exists(filename))
                        File.Delete(filename);

                    store.Remove(id);

                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteSuccess, id));
                }
                catch (Exception ex)
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.GetBaseException().Message));
                }
                finally
                {
                    semaphore.Release();
                }
            }
            catch (Exception)
            {
            }
        }

        public async Task DelAllFingerPrints()
        {
            try
            {
                await semaphore.WaitAsync();

                try
                {
                    Directory.GetFiles(StorePath).ToList().ForEach(File.Delete);

                    store.Clear();

                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteAllSuccess));

                }
                catch (Exception ex)
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.GetBaseException().Message));
                }
                finally
                {
                    semaphore.Release();
                }
            }
            catch (Exception)
            {
            }
        }

        public Task<int> GetFingerPrintCount()
        {
            var result = store.Count;

            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.GetCountSuccess, result));

            return Task.FromResult(result);
        }

        private int StoreIdentify(byte[] temp)
        {
            int id = -1;
            int maxScore = 0;

            foreach (var storeItem in store)
            {
                int score = fpengine.MatchTemplateOne(temp, storeItem.Value, 512);

                if (score >= 80)
                {
                    if (score > maxScore)
                        id = storeItem.Key;
                }
            }

            return id;
        }

        private void Restore()
        {
            try
            {
                store.Clear();

                if (Directory.Exists(StorePath))
                    foreach (var file in Directory.GetFiles(StorePath, searchPattern))
                    {
                        if (disposedValue)
                        {
                            return;
                        }

                        var bytes = File.ReadAllBytes(file);

                        int fid = StoreIdentify(bytes);

                        if (fid >= 0)
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                        }
                        else
                        {
                            var match = extractIdFromFileNamePattern.Match(file);

                            if (match.Success)
                            {
                                var id = int.Parse(match.Groups[1].Value);

                                if (!store.ContainsKey(id))
                                {
                                    store.Add(id, bytes);

                                    id++;

                                    if (id > FingerPrintId)
                                        FingerPrintId = id;
                                }
                                else
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, id));
                            }
                        }
                    }
                else
                    Directory.CreateDirectory(StorePath);
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ImportError, ex.GetBaseException().Message));
            }
        }

        private void Import(bool singleFile = false)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(ImportPath) && !Directory.Exists(ImportPath))
                    Directory.CreateDirectory(ImportPath);

                if (!string.IsNullOrWhiteSpace(StorePath) && !Directory.Exists(StorePath))
                    Directory.CreateDirectory(StorePath);

                foreach (var filePath in Directory.GetFiles(ImportPath, searchPattern))
                {
                    if (token.IsCancellationRequested || disposedValue)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.OperationCancelled));
                        return;
                    }

                    var bytes = File.ReadAllBytes(filePath);

                    int fid = StoreIdentify(bytes);

                    if (fid >= 0)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                    }
                    else
                    {
                        if (!store.ContainsKey(FingerPrintId))
                        {
                            store.Add(FingerPrintId, bytes);

                            var dest = filePath.Replace(Path.GetFileName(filePath), $"Template-{FingerPrintId}.lr.bak");

                            if (StorePath != null)
                                dest = $@"{StorePath}\Template-{FingerPrintId}.lr.fpt";

                            if (File.Exists(dest))
                                File.Delete(dest);

                            File.Move(filePath, dest);

                            FingerPrintId++;
                        }
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, FingerPrintId));
                    }

                    if (singleFile)
                        return;
                }
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ImportError, ex.Message));
            }
        }

        public Task ImportFingerPrints()
        {
            refreshToken();

            return Task.Run(() =>
            {
                Import(false);
            });
        }

        private void SaveTemplate(byte[] tmp, int tmpN, bool duplicate = false)
        {
            string filename = $@"{StorePath}\{string.Format(FileNameModel, tmpN)}";

            try
            {
                File.WriteAllBytes(filename, tmp);
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.Message));
                return;
            }

            if (!duplicate)
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.StoreSuccess, tmpN));

        }

        #region IDisposable Support
        private bool disposedValue = false;

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    tokenSource?.Dispose();

                    IsTimeToDie = true;
                    Thread.Sleep(1000);
                    fpengine.CloseDevice();
                }

                disposedValue = true;
            }
        }
        public void Dispose()
        {
            Dispose(true);
        }
        #endregion
    }
}
