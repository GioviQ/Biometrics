using System;
using System.Threading.Tasks;

namespace Biometrics
{
    public interface IFingerPrintHelper
    {
        bool IsConnected { get; }
        string StorePath { get; set; }
        string ImportPath { get; set; }
        string FileNameModel { get; set; }

        event EventHandler<FingerPrintEventArgs> Notification;

        void CancelOperation();
        Task DelAllFingerPrints();
        Task DelFingerPrint(int id);
        void Dispose();
        Task<int> GetFingerPrintCount();
        Task Identify();
        Task ImportFingerPrints();
        Task Start(string deviceConnection = "USB");
        Task Store();
    }
}