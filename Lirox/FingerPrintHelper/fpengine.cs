using System;
using System.Runtime.InteropServices;

namespace Biometrics
{
    class fpengine
    {
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SetMsgMainHandle(IntPtr hWnd);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SetTimeOut(double itime);

        //Open Device
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int OpenDevice(int nPortNum, int nPortPara, int nDeviceType);

        //Link Device
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int LinkDevice(UInt32 pw);

        //Close Device
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int CloseDevice();

        //Capture image
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void CaptureImage();

        //Capture Image and Feature (finger collection once)
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void CaptureTemplate();

        //Enrol Image and Feature（Two finger collections）
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void EnrollTemplate();

        //Getting Work Message
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetWorkMsg();

        //Get Work Return
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetRetMsg();

        //Release Message
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int ReleaseMsg();

        //Get Feature
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetTemplateByCap(byte[] pRefVal, ref int pRefSize);

        //Get Feature
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetTemplateByEnl(byte[] pRefVal, ref int pRefSize);

        //Get Feature（Ansi String）
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetBase64StrByCap(int unicode,byte[] pRefVal);

        //Get Feature（Ansi String）
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetBase64StrByEnl(int unicode, byte[] pRefVal);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int CreateTemplate(byte[] pFingerData, byte[] pTemplate);

        //Match Feature
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int MatchTemplate(byte[] pSrcData, byte[] pDstData);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int MatchTemplateOne(byte[] pSrcData, byte[] pDstData,int dstsize);


        //Match Feature（Ansi String）
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        //public static extern int MatchTemplateEx(byte[] pSrcData, byte[] pDstData);
        public static extern int MatchBase64Str(byte[] pSrcData, byte[] pDstData, int unicode);

        //Get Image (RAW)
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetRawImage(byte[] imagedata, ref int size);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SetImage(byte[] imagedata, int size);

        //Draw Image
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int DrawImage(IntPtr hdc, int left, int top);


        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int WriteUserInfo(byte[] databuf, int datasize);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int ReadUserInfo(byte[] databuf, int datasize);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern UInt32 GetDeviceSnNum();

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetDeviceSnStr(int unicode, byte[] hexstr);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetDeviceName(int unicode, byte[] devname);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetDeviceDate(int unicode, byte[] devdate);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetImageSize(ref int width, ref int height);

        //WSQ
        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int RawToWsq(byte[] rawdata, int width, int height, int depth, int dpi, float bitrate, byte[] wsqdata, ref int wsqsize);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int WsqToRaw(byte[] wsqdata, int wsqsize, byte[] rawdata, ref int width, ref int height, ref int depth, ref int dpi);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int BmpToWsq(byte[] rawdata, int width, int height, int depth, int dpi, float bitrate, byte[] wsqdata, ref int wsqsize);

        [DllImport("fpengine.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int WsqToBmp(byte[] wsqdata, int wsqsize, byte[] rawdata, ref int width, ref int height, ref int depth, ref int dpi);

        public const int FPDATASIZE = 256;
        public const int IMGWIDTH = 256;
        public const int IMGHEIGHT = 288;
        public const int IMGSIZE = 73728;

        public const int WM_FPMESSAGE = 1024 + 120;             
        public const int FPM_DEVICE=0x01;			
        public const int FPM_PLACE=0x02;			
        public const int FPM_LIFT=0x03;			  
        public const int FPM_CAPTURE=0x04;		
        public const int FPM_ENROLL=0x06;			
        public const int FPM_GENCHAR=0x05 ;    
        public const int FPM_NEWIMAGE=0x07;
        public const int FPM_TIMEOUT = 0x08;
        public const int RET_OK=0x1;
        public const int RET_FAIL=0x0;
           
    }
}
