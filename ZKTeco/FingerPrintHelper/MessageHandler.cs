using System;
using System.Windows.Forms;

namespace Biometrics
{
    internal class MessageHandler : NativeWindow
    {
        public event EventHandler<Message> MessageReceived;

        public MessageHandler()
        {
            CreateHandle(new CreateParams());
        }

        protected override void WndProc(ref Message msg)
        {
            MessageReceived?.Invoke(this, msg);

            base.WndProc(ref msg);
        }
    }
}
