using FingerPrintSDK;
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Biometrics
{
    public class FingerPrintHelperHoltek : IDisposable, IFingerPrintHelper
    {
        private readonly FingerPrintManager fpm;
        public event EventHandler<FingerPrintEventArgs> Notification;

        private CancellationTokenSource tokenSource;
        private CancellationToken token;

        private static readonly SemaphoreSlim semaphore = new(1);

        public bool IsConnected { get; private set; }
        public string StorePath { get; set; }
        public string ImportPath { get; set; }
        public string FileNameModel { get; set; } = "Template-{0}.fpt";

        public FingerPrintHelperHoltek()
        {
            tokenSource = new CancellationTokenSource();
            token = tokenSource.Token;

            fpm = new FingerPrintManager();
        }

        protected virtual void RaiseNotification(FingerPrintEventArgs e)
        {
            Notification?.Invoke(this, e);
        }

        private string GetErrorMsg(ErrorCode errorCode)
        {
            string errMsg;

            switch (errorCode)
            {
                case ErrorCode.SUCCESS:
                    errMsg = "Successo";
                    break;
                case ErrorCode.VERIFY:
                    errMsg = "Verifica fallita";
                    break;
                case ErrorCode.IDENTIFY:
                    errMsg = "Identificazione fallita";
                    break;
                case ErrorCode.EMPTY_ID_NOEXIST:
                    errMsg = "Non esiste un template vuoto";
                    break;
                case ErrorCode.BROKEN_ID_NOEXIST:
                    errMsg = "Non esiste un template corrotto";
                    break;
                case ErrorCode.TMPL_NOT_EMPTY:
                    errMsg = "Esiste già un template con questo ID";
                    break;
                case ErrorCode.TMPL_EMPTY:
                    errMsg = "Memoria vuota";
                    break;
                case ErrorCode.INVALID_TMPL_NO:
                    errMsg = "Numero template non valido";
                    break;
                case ErrorCode.ALL_TMPL_EMPTY:
                    errMsg = "Tutti i template sono vuoti";
                    break;
                case ErrorCode.INVALID_TMPL_DATA:
                    errMsg = "Dati template non validi";
                    break;
                case ErrorCode.DUPLICATION_ID:
                    errMsg = "ID duplicato";
                    break;
                case ErrorCode.BAD_QUALITY:
                    errMsg = "Scansione di bassa qualità";
                    break;
                case ErrorCode.MERGE_FAIL:
                    errMsg = "Unione fallita";
                    break;
                case ErrorCode.NOT_AUTHORIZED:
                    errMsg = "Dispositivo non autorizzato";
                    break;
                case ErrorCode.MEMORY:
                    errMsg = "Errore di memoria";
                    break;
                case ErrorCode.INVALID_PARAM:
                    errMsg = "Parametro non valido";
                    break;
                case ErrorCode.GEN_COUNT:
                    errMsg = "Il conteggio non è valido";
                    break;
                case ErrorCode.INVALID_BUFFER_ID:
                    errMsg = "ID del buffer non valido.";
                    break;
                case ErrorCode.INVALID_OPERATION_MODE:
                    errMsg = "Modalità operativa non valida";
                    break;
                case ErrorCode.FP_NOT_DETECTED:
                    errMsg = "Dito non rilevato";
                    break;
                default:
                    errMsg = String.Format("Errore {0}", errorCode);
                    break;
            }

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
            if (tokenSource != null)
                tokenSource.Cancel();
        }

        public Task Start(string deviceConnection = "USB")
        {
            return Task.Run(() =>
            {
                if (deviceConnection == "USB")
                    IsConnected = fpm.InitConnection(ConnectionMode.USB, string.Empty, 0);
                else
                    IsConnected = fpm.InitConnection(ConnectionMode.SERIAL, deviceConnection, BaudRates.BAUD115200);

                RaiseNotification(new FingerPrintEventArgs(IsConnected ? FingerPrintEvent.ConnectionSuccess : FingerPrintEvent.ConnectionFail));
            });
        }

        public Task Store()
        {
            return Task.Run(async () =>
            {
                try
                {
                    await semaphore.WaitAsync();

                    try
                    {
                        refreshToken();

                        if (!Directory.Exists(StorePath))
                            Directory.CreateDirectory(StorePath);


                        if (fpm.GetEmptyID(out int emptyID))
                        {
                            fpm.SLEDControl(1);

                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger, emptyID));

                            int enrollStep = 0, enrollStepCount = 3;

                            while (enrollStep < enrollStepCount && !disposedValue)
                            {
                                for (; ; )
                                {
                                    var resI = fpm.GetImage();

                                    if (resI == (int)ErrorCode.SUCCESS)
                                        break;
                                    else if (resI == (int)ErrorCode.CONNECTION)
                                    {
                                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                                        return;
                                    }

                                    if (token.IsCancellationRequested || disposedValue)
                                    {
                                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.OperationCancelled));
                                        return;
                                    }
                                }

                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.RaiseFinger, emptyID));

                                var res = (ErrorCode)fpm.Generate(enrollStep);

                                if (res != ErrorCode.SUCCESS)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));

                                    if (res == ErrorCode.BAD_QUALITY)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        return;
                                    }
                                }

                                enrollStep++;

                                if (enrollStep < enrollStepCount)
                                {
                                    await Task.Delay(3000);
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.PutFinger, emptyID));
                                }
                            }

                            if (enrollStepCount != 1)
                            {
                                var res = (ErrorCode)fpm.Merge(0, enrollStepCount);

                                if (res != ErrorCode.SUCCESS)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));
                                    return;
                                }
                            }


                            var ret = (ErrorCode)fpm.StoreChar(emptyID, 0, out int dupID);

                            if (ret != ErrorCode.SUCCESS)
                            {
                                if (ret == ErrorCode.DUPLICATION_ID)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate, dupID));
                                    SaveTemplate(dupID, true);
                                }
                                else
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
                                    return;
                                }
                            }
                            else
                                SaveTemplate(emptyID);

                            fpm.SLEDControl(0);
                        }
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.MemoryFull));
                    }
                    finally
                    {
                        semaphore.Release();
                    }
                }
                catch (Exception)
                {
                }
            });
        }

        public Task Identify()
        {
            return Task.Run(async () =>
            {
                try
                {
                    await semaphore.WaitAsync();

                    try
                    {
                        refreshToken();

                        for (; ; )
                        {
                            if (token.IsCancellationRequested || disposedValue)
                            {
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.OperationCancelled));
                                return;
                            }

                            if (ImportPath != null)
                                Import(true);

                            for (; ; )
                            {
                                if (token.IsCancellationRequested)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.OperationCancelled));
                                    return;
                                }

                                var resI = fpm.GetImage();

                                if (resI == (int)ErrorCode.SUCCESS)
                                    break;
                                else if (resI == (int)ErrorCode.CONNECTION)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.ConnectionFail));
                                    return;
                                }
                            }

                            var res = (ErrorCode)fpm.Generate(0);

                            if (res != ErrorCode.SUCCESS)
                            {
                                if (res == ErrorCode.CONNECTION)
                                {
                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));
                                    return;
                                }
                                else
                                    await Task.Delay(1000, token);
                            }

                            res = (ErrorCode)fpm.Search(out int tmplN, out int learnResult);

                            if (res == ErrorCode.SUCCESS)
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifySuccess, tmplN));
                            else if (res == ErrorCode.IDENTIFY)
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.IdentifyFail));
                            else
                                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));
                        }
                    }
                    finally
                    {
                        semaphore.Release();
                    }
                }
                catch (Exception)
                {
                }
            });
        }

        public Task DelFingerPrint(int id)
        {
            return Task.Run(async () =>
            {
                try
                {
                    await semaphore.WaitAsync();

                    try
                    {
                        var ret = (ErrorCode)fpm.DelChar(id);

                        if (ret == ErrorCode.SUCCESS)
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteSuccess, id));
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
                    }
                    finally
                    {
                        semaphore.Release();
                    }
                }
                catch (Exception)
                {
                }

            });
        }

        public Task DelAllFingerPrints()
        {
            return Task.Run(async () =>
            {
                try
                {
                    await semaphore.WaitAsync();

                    try
                    {
                        var ret = (ErrorCode)fpm.DelAll();

                        if (ret == ErrorCode.SUCCESS)
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.DeleteAllSuccess));
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
                    }
                    finally
                    {
                        semaphore.Release();
                    }
                }
                catch (Exception)
                {
                }

            });
        }

        public async Task<int> GetFingerPrintCount()
        {
            try
            {
                await semaphore.WaitAsync();

                try
                {
                    var ret = (ErrorCode)fpm.GetEnrollCount(out int count);

                    if (ret == ErrorCode.SUCCESS)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.GetCountSuccess, count));
                        return count;
                    }
                    else
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
                        return -1;
                    }
                }
                catch (Exception ex)
                {
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.GetBaseException().Message));

                    return -1;
                }
                finally
                {
                    semaphore.Release();
                }
            }
            catch (Exception)
            {
                return -1;
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

                foreach (var file in Directory.GetFiles(ImportPath, "*.fpt"))
                {
                    if (token.IsCancellationRequested || disposedValue)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.OperationCancelled));
                        return;
                    }

                    if (fpm.GetEmptyID(out int emptyID))
                    {
                        var bytes = File.ReadAllBytes(file);

                        var res = (ErrorCode)fpm.DownChar(0, bytes);

                        if (res == ErrorCode.SUCCESS)
                        {
                            res = (ErrorCode)fpm.StoreChar(emptyID, 0, out int dupID);

                            if (res != ErrorCode.SUCCESS)
                            {
                                if (res == ErrorCode.DUPLICATION_ID)
                                {
                                    emptyID = dupID;

                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.FingerPrintDuplicate));
                                }
                                else

                                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));
                            }

                            if (res == ErrorCode.SUCCESS || res == ErrorCode.DUPLICATION_ID)
                            {
                                var dest = file.Replace(Path.GetFileName(file), $"Template-{emptyID}.bak");

                                if (StorePath != null)
                                    dest = $@"{StorePath}\Template-{emptyID}.fpt";

                                if (File.Exists(dest))
                                    File.Delete(dest);

                                File.Move(file, dest);
                            }
                        }
                        else
                            RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(res)));
                    }
                    else
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.MemoryFull));
                        return;
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

        private void SaveTemplate(int tmpN, bool duplicate = false)
        {
            //carica nel buffer con id 0 il template con id tmpN
            var ret = (ErrorCode)fpm.LoadChar(tmpN, 0);

            if (ret != ErrorCode.SUCCESS)
                RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
            else
            {

                //carica in bytes il contenuto del buffer con id 0
                ret = (ErrorCode)fpm.UpChar(0, out byte[] bytes);

                if (ret != ErrorCode.SUCCESS)
                    RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, GetErrorMsg(ret)));
                else
                {
                    string filename = $@"{StorePath}\{string.Format(FileNameModel, tmpN)}";

                    try
                    {
                        File.WriteAllBytes(filename, bytes);
                    }
                    catch (Exception ex)
                    {
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.Error, ex.Message));
                        return;
                    }

                    if (!duplicate)
                        RaiseNotification(new FingerPrintEventArgs(FingerPrintEvent.StoreSuccess, tmpN));
                }
            }
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

                    fpm.Dispose();
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
