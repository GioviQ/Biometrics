using libzkfpcsharp;
using System;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Biometrics
{
    public class FingerPrintHelperZK : IDisposable, IFingerPrintHelper
    {
        public event EventHandler<FingerPrintEventArgs> Notification;

        private CancellationTokenSource tokenSource;
        private CancellationToken token;

        private static SemaphoreSlim semaphore = new SemaphoreSlim(1);

        public bool IsConnected { get; private set; }
        public string StorePath { get; set; }
        public string ImportPath { get; set; }
        public string FileNameModel { get; set; } = "Template-{0}.zk.fpt";

        private const string searchPattern = "*.zk.fpt";

        private IntPtr mDevHandle = IntPtr.Zero;
        private IntPtr mDBHandle = IntPtr.Zero;

        private bool IsTimeToDie = false;
        private bool IsRegister = false;
        private bool toIdentify = true;

        private byte[] FPBuffer;

        private int RegisterCount = 0;
        private const int REGISTER_FINGER_COUNT = 3;

        private readonly byte[][] RegTmps = new byte[3][];
        private readonly byte[] RegTmp = new byte[2048];
        private readonly byte[] CapTmp = new byte[2048];

        private int cbCapTmp = 2048;
        private int cbRegTmp = 0;
        private int FingerPrintId = 1;

        private int mfpWidth = 0;
        private int mfpHeight = 0;
        private int mfpDpi = 0;

        private const int MESSAGE_CAPTURED_OK = 0x0400 + 6;

        [DllImport("user32.dll", EntryPoint = "SendMessageA")]
        public static extern int SendMessage(IntPtr hwnd, int wMsg, IntPtr wParam, IntPtr lParam);

        private readonly Regex extractIdFromFileNamePattern = new Regex(@"Template-(\d+).zk.fpt");

        public FingerPrintHelperZK()
        {
            tokenSource = new CancellationTokenSource();
            token = tokenSource.Token;
        }

        protected virtual void RaiseNotification(FingerPrintEventArgs e)
        {
            Notification?.Invoke(this, e);
        }

        private string GetMsg(int errorCode)
        {
            string errMsg;

            if (errorCode == zkfp.ZKFP_ERR_ALREADY_INIT)
                errMsg = "Initialized";
            else if (errorCode == zkfp.ZKFP_ERR_OK)
                errMsg = "Operation succeeded";
            else if (errorCode == zkfp.ZKFP_ERR_INITLIB)
                errMsg = "Failed to initialize the algorithm library";
            else if (errorCode == zkfp.ZKFP_ERR_INIT)
                errMsg = "Failed to initialize the capture library";
            else if (errorCode == zkfp.ZKFP_ERR_NO_DEVICE)
                errMsg = "No device connected";
            else if (errorCode == zkfp.ZKFP_ERR_NOT_SUPPORT)
                errMsg = "Not supported by the interface";
            else if (errorCode == zkfp.ZKFP_ERR_INVALID_PARAM)
                errMsg = "Invalid parameter";
            else if (errorCode == zkfp.ZKFP_ERR_OPEN)
                errMsg = "Failed to start the device";
            else if (errorCode == zkfp.ZKFP_ERR_INVALID_HANDLE)
                errMsg = "Invalid handle";
            else if (errorCode == zkfp.ZKFP_ERR_CAPTURE)
                errMsg = "Failed to capture the image";
            else if (errorCode == zkfp.ZKFP_ERR_EXTRACT_FP)
                errMsg = "Failed to extract the fingerprint template";
            else if (errorCode == zkfp.ZKFP_ERR_ABSORT)
                errMsg = "Suspension";
            else if (errorCode == zkfp.ZKFP_ERR_MEMORY_NOT_ENOUGH)
                errMsg = "Insufficient memory";
            else if (errorCode == zkfp.ZKFP_ERR_BUSY)
                errMsg = "The fingerprint is being captured";
            else if (errorCode == zkfp.ZKFP_ERR_ADD_FINGER)
                errMsg = "Failed to add the fingerprint template";
            else if (errorCode == zkfp.ZKFP_ERR_DEL_FINGER)
                errMsg = "Failed to delete the fingerprint template";
            else if (errorCode == zkfp.ZKFP_ERR_FAIL)
                errMsg = "Operation failed";
            else if (errorCode == zkfp.ZKFP_ERR_CANCEL)
                errMsg = "Capture cancelled";
            else if (errorCode == zkfp.ZKFP_ERR_VERIFY_FP)
                errMsg = "Fingerprint comparison failed";
            else if (errorCode == zkfp.ZKFP_ERR_MERGE)
                errMsg = "Failed to combine registered fingerprint templates";
            else if (errorCode == zkfp.ZKFP_ERR_NOT_OPENED)
                errMsg = "Device not started";
            else if (errorCode == zkfp.ZKFP_ERR_NOT_INIT)
                errMsg = "Not initialized";
            else if (errorCode == zkfp.ZKFP_ERR_ALREADY_OPENED)
                errMsg = "Device started";
            else
                errMsg = string.Format("Error code {0}", errorCode);

            return errMsg;
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

        public Task Start(string deviceConnection = "USB")
        {
            try
            {
                if (zkfp2.Init() == zkfperrdef.ZKFP_ERR_OK)
                {
                    int nCount = zkfp2.GetDeviceCount();
                    if (nCount > 0)
                    {
                        if (IntPtr.Zero == (mDevHandle = zkfp2.OpenDevice(0)))
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                            return Task.CompletedTask;
                        }

                        if (IntPtr.Zero == (mDBHandle = zkfp2.DBInit()))
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                            zkfp2.CloseDevice(mDevHandle);
                            mDevHandle = IntPtr.Zero;
                            return Task.CompletedTask;
                        }

                        RegisterCount = 0;
                        cbRegTmp = 0;
                        FingerPrintId = 1;

                        for (int i = 0; i < 3; i++)
                        {
                            RegTmps[i] = new byte[2048];
                        }

                        byte[] paramValue = new byte[4];
                        int size = 4;
                        zkfp2.GetParameters(mDevHandle, 1, paramValue, ref size);
                        zkfp2.ByteArray2Int(paramValue, ref mfpWidth);

                        size = 4;
                        zkfp2.GetParameters(mDevHandle, 2, paramValue, ref size);
                        zkfp2.ByteArray2Int(paramValue, ref mfpHeight);

                        FPBuffer = new byte[mfpWidth * mfpHeight];

                        size = 4;
                        zkfp2.GetParameters(mDevHandle, 3, paramValue, ref size);
                        zkfp2.ByteArray2Int(paramValue, ref mfpDpi);

                        Thread captureThread = new Thread(new ThreadStart(DoCapture))
                        {
                            IsBackground = true
                        };

                        Restore();

                        captureThread.Start();

                        IsTimeToDie = false;

                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionSuccess));
                    }
                    else
                    {
                        zkfp2.Terminate();
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                    }
                }
                else
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
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
            MessageHandler messageHandler = new MessageHandler();

            messageHandler.MessageReceived += MessageHandler_MessageReceived;

            while (!IsTimeToDie)
            {
                cbCapTmp = 2048;

                int ret = zkfp2.AcquireFingerprint(mDevHandle, FPBuffer, CapTmp, ref cbCapTmp);

                if (ret == zkfp.ZKFP_ERR_OK)
                {
                    SendMessage(messageHandler.Handle, MESSAGE_CAPTURED_OK, IntPtr.Zero, IntPtr.Zero);
                }

                Thread.Sleep(200);

                Import(true);
            }
        }

        private void MessageHandler_MessageReceived(object sender, Message m)
        {
            switch (m.Msg)
            {
                case MESSAGE_CAPTURED_OK:
                    {
                        MemoryStream ms = new MemoryStream();
                        BitmapFormat.GetBitmap(FPBuffer, mfpWidth, mfpHeight, ref ms);

                        if (IsRegister)
                        {
                            int fid = 0, score = 0;
                            int ret = zkfp2.DBIdentify(mDBHandle, CapTmp, ref fid, ref score);

                            if (zkfp.ZKFP_ERR_OK == ret)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                                return;
                            }

                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.RaiseFinger));

                            if (RegisterCount > 0 && zkfp2.DBMatch(mDBHandle, CapTmp, RegTmps[RegisterCount - 1]) <= 0)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
                                return;
                            }

                            Array.Copy(CapTmp, RegTmps[RegisterCount], cbCapTmp);

                            RegisterCount++;

                            if (RegisterCount >= REGISTER_FINGER_COUNT)
                            {
                                if (!Directory.Exists(StorePath))
                                    Directory.CreateDirectory(StorePath);

                                RegisterCount = 0;
                                if (zkfp.ZKFP_ERR_OK == (ret = zkfp2.DBMerge(mDBHandle, RegTmps[0], RegTmps[1], RegTmps[2], RegTmp, ref cbRegTmp)) &&
                                    zkfp.ZKFP_ERR_OK == (ret = zkfp2.DBAdd(mDBHandle, FingerPrintId, RegTmp)))
                                {
                                    var bytesTosave = new byte[cbRegTmp];
                                    Array.Copy(RegTmp, bytesTosave, cbRegTmp);
                                    SaveTemplate(bytesTosave, FingerPrintId);
                                    FingerPrintId++;
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
                                }

                                IsRegister = false;

                                return;
                            }
                            else
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
                            }
                        }
                        else
                        {
                            if (FingerPrintId <= 1)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifyFail, "Empty memory"));
                                return;
                            }

                            if (toIdentify)
                            {
                                int fid = 0, score = 0;
                                int ret = zkfp2.DBIdentify(mDBHandle, CapTmp, ref fid, ref score);

                                if (zkfp.ZKFP_ERR_OK == ret)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifySuccess, fid));
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifyFail, GetMsg(ret)));
                                }

                                return;
                            }
                            else
                            {
                                int ret = zkfp2.DBMatch(mDBHandle, CapTmp, RegTmp);

                                if (0 < ret)
                                {
                                    return;
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
                                    return;
                                }
                            }
                        }
                    }
                    break;
            }
        }

        public Task Store()
        {
            if (!IsRegister)
            {
                IsRegister = true;
                RegisterCount = 0;
                cbRegTmp = 0;

                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger));
            }

            return Task.CompletedTask;
        }

        public Task Identify()
        {
            if (!toIdentify)
            {
                toIdentify = true;
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

                    int ret = zkfp2.DBDel(mDBHandle, id);

                    if (zkfp.ZKFP_ERR_OK == ret)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteSuccess, id));
                    }
                    else
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
                    }
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

                    int ret = zkfp2.DBFree(mDBHandle);

                    if (zkfp.ZKFP_ERR_OK == ret)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteAllSuccess));
                    }
                    else
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
                    }
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
            var result = FingerPrintId - 1;

            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.GetCountSuccess, FingerPrintId - 1));

            return Task.FromResult(result);
        }

        private void Restore()
        {
            try
            {
                if (Directory.Exists(StorePath))
                    foreach (var file in Directory.GetFiles(StorePath, searchPattern))
                    {
                        if (disposedValue)
                        {
                            return;
                        }

                        var bytes = File.ReadAllBytes(file);

                        int fid = 0, score = 0;
                        int ret = zkfp2.DBIdentify(mDBHandle, bytes, ref fid, ref score);

                        if (zkfp.ZKFP_ERR_OK == ret)
                        {
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                        }
                        else
                        {
                            var match = extractIdFromFileNamePattern.Match(file);

                            if (match.Success)
                            {
                                var id = int.Parse(match.Groups[1].Value);

                                ret = zkfp2.DBAdd(mDBHandle, id, bytes);

                                if (zkfp.ZKFP_ERR_OK == ret)
                                {
                                    id++;

                                    if (id > FingerPrintId)
                                        FingerPrintId = id;
                                }
                                else
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
                            }
                        }
                    }
                else
                    Directory.CreateDirectory(StorePath);
            }
            catch (Exception ex)
            {
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ImportError, ex.Message));
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

                    int fid = 0, score = 0;
                    int ret = zkfp2.DBIdentify(mDBHandle, bytes, ref fid, ref score);

                    if (zkfp.ZKFP_ERR_OK == ret)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, fid));
                    }
                    else
                    {
                        ret = zkfp2.DBAdd(mDBHandle, FingerPrintId, bytes);

                        if (zkfp.ZKFP_ERR_OK == ret)
                        {
                            var dest = filePath.Replace(Path.GetFileName(filePath), $"Template-{FingerPrintId}.zk.bak");

                            if (StorePath != null)
                                dest = $@"{StorePath}\Template-{FingerPrintId}.zk.fpt";

                            if (File.Exists(dest))
                                File.Delete(dest);

                            File.Move(filePath, dest);

                            FingerPrintId++;
                        }
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetMsg(ret)));
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
                    RegisterCount = 0;
                    Thread.Sleep(1000);
                    zkfp2.CloseDevice(mDevHandle);
                    zkfp2.Terminate();
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
