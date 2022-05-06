namespace Biometrics
{
    public enum FingerPrintEvent
    {
        ConnectionSuccess,
        ConnectionFail,
        PutFinger,
        RaiseFinger,
        Error,
        StoreSuccess,
        IdentifySuccess,
        IdentifyFail,
        DeleteSuccess,
        DeleteAllSuccess,
        MemoryFull,
        FingerPrintDuplicate,
        OperationCancelled,
        GetCountSuccess,
        ImportError,
    }
}
