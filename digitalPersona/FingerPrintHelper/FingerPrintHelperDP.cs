using DPUruNet;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace Biometrics
{
    public class FingerPrintHelperDP : IDisposable, IFingerPrintHelper
    {
        public event EventHandler<FingerPrintEventArgs> Notification;

        private CancellationTokenSource tokenSource;
        private CancellationToken token;

        private static SemaphoreSlim semaphore = new SemaphoreSlim(1);

        private Reader currentReader;
        public bool IsConnected { get; private set; }
        public string StorePath { get; set; }
        public string ImportPath { get; set; }
        public string FileNameModel { get; set; } = "Template-{0}.dp.fpt";

        private const string searchPattern = "*.dp.fpt";

        private const int DPFJ_PROBABILITY_ONE = 0x7fffffff;

        private readonly Dictionary<int, Fmd> store = new Dictionary<int, Fmd>();
        private readonly List<Fmd> preenrollmentFmds = new List<Fmd>();

        private bool IsRegister = false;
        private bool toIdentify = true;

        private int RegisterCount = 0;
        private const int REGISTER_FINGER_COUNT = 4;


        private int FingerPrintId = 1;

        private readonly Regex extractIdFromFileNamePattern = new Regex(@"Template-(\d+).dp.fpt");

        public FingerPrintHelperDP()
        {
            tokenSource = new CancellationTokenSource();
            token = tokenSource.Token;
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
            IsRegister = false;
            RegisterCount = 0;

            if (tokenSource != null)
                tokenSource.Cancel();
        }

        public async Task Start(string deviceConnection = "USB")
        {
            try
            {
                var readers = ReaderCollection.GetReaders();

                if (readers.Count == 0)
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail, "No finger print scanner detected"));
                else
                {
                    currentReader = readers[0];

                    Constants.ResultCode result = Constants.ResultCode.DP_DEVICE_FAILURE;

                    result = currentReader.Open(Constants.CapturePriority.DP_PRIORITY_COOPERATIVE);

                    if (result != Constants.ResultCode.DP_SUCCESS)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail, $"Error: {result}"));
                    }
                    else
                    {
                        Restore();
                        await Import();

                        currentReader.On_Captured += new Reader.CaptureCallback(OnCaptured);

                        GetStatus();

                        Constants.ResultCode captureResult = currentReader.CaptureAsync(Constants.Formats.Fid.ANSI, Constants.CaptureProcessing.DP_IMG_PROC_DEFAULT, currentReader.Capabilities.Resolutions[0]);

                        if (captureResult != Constants.ResultCode.DP_SUCCESS)
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail, $"Error: {captureResult}"));
                        else
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionSuccess));

                            var watcher = new FileSystemWatcher(ImportPath);

                            watcher.Changed += Watcher_Changed;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail, ex.GetBaseException().Message));
            }
        }

        private void Watcher_Changed(object sender, FileSystemEventArgs e)
        {
            _ = Import();
        }

        private void GetStatus()
        {
            Constants.ResultCode result = currentReader.GetStatus();

            if (result != Constants.ResultCode.DP_SUCCESS)
            {
                throw new Exception(result.ToString());
            }

            if (currentReader.Status.Status == Constants.ReaderStatuses.DP_STATUS_BUSY)
            {
                Thread.Sleep(50);
            }
            else if (currentReader.Status.Status == Constants.ReaderStatuses.DP_STATUS_NEED_CALIBRATION)
            {
                currentReader.Calibrate();
            }
            else if (currentReader.Status.Status != Constants.ReaderStatuses.DP_STATUS_READY)
            {
                throw new Exception($"Reader Status - {currentReader.Status.Status}");
            }
        }

        public bool CheckCaptureResult(CaptureResult captureResult)
        {
            if (captureResult.Data == null || captureResult.ResultCode != Constants.ResultCode.DP_SUCCESS)
            {
                if (captureResult.ResultCode != Constants.ResultCode.DP_SUCCESS)
                {
                    throw new Exception(captureResult.ResultCode.ToString());
                }

                // Send message if quality shows fake finger
                if (captureResult.Quality != Constants.CaptureQuality.DP_QUALITY_CANCELED)
                {
                    throw new Exception($"Quality - {captureResult.Quality}");
                }

                return false;
            }

            return true;
        }

        private void OnCaptured(CaptureResult captureResult)
        {
            try
            {
                if (CheckCaptureResult(captureResult))
                {
                    DataResult<Fmd> resultConversion = FeatureExtraction.CreateFmdFromFid(captureResult.Data, Constants.Formats.Fmd.ANSI);

                    if (resultConversion.ResultCode != Constants.ResultCode.DP_SUCCESS)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, resultConversion.ResultCode.ToString()));
                    }
                    else if (IsRegister)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.RaiseFinger));

                        int fid = StoreIdentify(resultConversion.Data);

                        if (fid >= 0)
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                        }
                        else
                        {
                            RegisterCount++;

                            preenrollmentFmds.Add(resultConversion.Data);

                            if (RegisterCount >= REGISTER_FINGER_COUNT)
                            {
                                if (!Directory.Exists(StorePath))
                                    Directory.CreateDirectory(StorePath);

                                DataResult<Fmd> resultEnrollment = Enrollment.CreateEnrollmentFmd(Constants.Formats.Fmd.ANSI, preenrollmentFmds);

                                if (resultEnrollment.ResultCode == Constants.ResultCode.DP_SUCCESS)
                                {
                                    SaveTemplate(resultEnrollment.Data, FingerPrintId);
                                    FingerPrintId++;
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, "Enroll fail"));
                                }

                                preenrollmentFmds.Clear();

                                IsRegister = false;

                                RegisterCount = 0;
                            }
                            else
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
                        }
                    }
                    else
                    {
                        int fid = StoreIdentify(resultConversion.Data);

                        if (fid >= 0)
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifySuccess, fid));
                        }
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifyFail));
                    }
                }
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.GetBaseException().Message));
            }
        }

        public Task Store()
        {
            if (!IsRegister)
            {
                preenrollmentFmds.Clear();

                IsRegister = true;

                RegisterCount = 0;

                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
            }

            return Task.CompletedTask;
        }

        public Task Identify()
        {
            if (!toIdentify)
            {
                toIdentify = true;

                preenrollmentFmds.Clear();

                IsRegister = false;

                RegisterCount = 0;
            }

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

        private int StoreIdentify(Fmd temp)
        {
            int id = -1;

            if (store.Count > 0)
            {
                // See the SDK documentation for an explanation on threshold scores.
                int thresholdScore = DPFJ_PROBABILITY_ONE * 1 / 100000;

                IdentifyResult identifyResult = Comparison.Identify(temp, 0, store.Values, thresholdScore, 1);

                if (identifyResult.ResultCode != Constants.ResultCode.DP_SUCCESS)
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, identifyResult.ResultCode.ToString()));
                }
                else if (identifyResult.Indexes.Length > 0)
                    id = store.Keys.ElementAt(identifyResult.Indexes[0][0]);
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

                        var fmd = Fmd.DeserializeXml(File.ReadAllText(file));

                        int fid = StoreIdentify(fmd);

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
                                    store.Add(id, fmd);

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

        private async Task Import(bool singleFile = false)
        {
            try
            {
                await semaphore.WaitAsync();

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

                        var fmd = Fmd.DeserializeXml(File.ReadAllText(filePath));

                        int fid = StoreIdentify(fmd);

                        if (fid >= 0)
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                        }
                        else
                        {
                            if (!store.ContainsKey(FingerPrintId))
                            {
                                store.Add(FingerPrintId, fmd);

                                var dest = filePath.Replace(Path.GetFileName(filePath), $"Template-{FingerPrintId}.dp.bak");

                                if (StorePath != null)
                                    dest = $@"{StorePath}\{string.Format(FileNameModel, FingerPrintId)}";

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
                finally
                {
                    semaphore.Release();
                }
            }
            catch (Exception)
            {
            }
        }

        public Task ImportFingerPrints()
        {
            refreshToken();

            return Task.Run(async () =>
            {
                await Import(false);
            });
        }

        private void SaveTemplate(Fmd tmp, int tmpN, bool duplicate = false)
        {
            string filename = $@"{StorePath}\{string.Format(FileNameModel, tmpN)}";

            try
            {
                File.WriteAllText(filename, Fmd.SerializeXml(tmp));
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

                    RegisterCount = 0;
                    currentReader.CancelCapture();
                    currentReader.Dispose();
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
