using System;

namespace Biometrics
{
    public class FingerPrintEventArgs : EventArgs
    {
        public FingerPrintEventArgs(FingerPrintEvent e)
        {
            Event = e;
        }

        public FingerPrintEventArgs(FingerPrintEvent e, string error)
        {
            Event = e;
            Error = error;
        }

        public FingerPrintEventArgs(FingerPrintEvent e, int temp)
        {
            Event = e;
            Temp = temp;
        }

        public FingerPrintEvent Event { get; }

        public String Error { get; }

        public int Temp { get; }
    }
}
