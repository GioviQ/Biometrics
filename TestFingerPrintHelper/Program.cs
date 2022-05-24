using Biometrics;
using Spectre.Console;
using System;
using System.IO;

namespace TestFingerPrintHelper
{
    class Program
    {
        static readonly string curPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
        static void Main()
        {
            ConsoleKeyInfo cki = default;

            var table = new Table
            {
                Title = new TableTitle("[yellow]FingerPrint Reader Tester[/]")
            };
            table.AddColumn("Selection");
            table.AddColumn("Device");
            table.AddRow("[green]1[/]", "Holtek");
            table.AddRow("[green]2[/]", "ZKTeco");
            table.AddRow("[green]3[/]", "LIROX");
            table.AddRow("[green]4[/]", "digitalPersona");

            AnsiConsole.Write(table);

            while (cki.Key != ConsoleKey.D1 && cki.Key != ConsoleKey.D2 && cki.Key != ConsoleKey.D3 && cki.Key != ConsoleKey.D4)
            {
                cki = Console.ReadKey(true);
            }

            IFingerPrintHelper fph = null;

            switch (cki.Key)
            {
                case ConsoleKey.D1:
                    fph = new FingerPrintHelperHoltek();
                    break;
                case ConsoleKey.D2:
                    fph = new FingerPrintHelperZK();
                    break;
                case ConsoleKey.D3:
                    fph = new FingerPrintHelperLirox();
                    break;
                case ConsoleKey.D4:
                    fph = new FingerPrintHelperDP();
                    break;
            }

            try
            {
                fph.StorePath = $@"{curPath}\Store";
                fph.ImportPath = $@"{curPath}\Import";

                fph.Notification += Fph_Notification;

                Console.WriteLine($"Inizializzazione FingerPrint {Properties.Settings.Default.Connection}");
                fph.Start(Properties.Settings.Default.Connection);

                table = new Table
                {
                    Title = new TableTitle("[yellow]Operations[/]")
                };
                table.AddColumn("Selection");
                table.AddColumn("Operation");
                table.AddRow("[green]I[/]", "Identify");
                table.AddRow("[green]S[/]", "Store");
                table.AddRow("[green]X[/]", "Exit");

                AnsiConsole.Write(table);

                do
                {
                    if (Console.KeyAvailable)
                    {
                        cki = Console.ReadKey(true);

                        switch (cki.Key)
                        {
                            case ConsoleKey.I:
                                fph.Identify();
                                break;
                            case ConsoleKey.S:
                                fph.Store();
                                break;
                        }
                    }
                } while (cki.Key != ConsoleKey.X);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }

        private static void Fph_Notification(object sender, FingerPrintEventArgs e)
        {
            IFingerPrintHelper fp = (IFingerPrintHelper)sender;

            switch (e.Event)
            {
                case FingerPrintEvent.Error:
                    Console.WriteLine(e.Error);
                    break;
                case FingerPrintEvent.IdentifyFail:
                    Console.WriteLine($"Identificazione fallita {e.Error}");
                    break;
                case FingerPrintEvent.PutFinger:
                    Console.WriteLine("Appoggiare il dito");
                    break;
                case FingerPrintEvent.RaiseFinger:
                    Console.WriteLine("Sollevare il dito");
                    break;
                case FingerPrintEvent.ConnectionSuccess:
                    Console.WriteLine("Inizializzazione riuscita");
                    break;
                case FingerPrintEvent.ConnectionFail:
                    Console.WriteLine("Connessione fallita");
                    break;
                case FingerPrintEvent.StoreSuccess:
                    Console.WriteLine("Impronta memorizzata");
                    break;
                case FingerPrintEvent.IdentifySuccess:
                    Console.WriteLine($"Impronta riconosciuta {e.Temp}");
                    break;
                case FingerPrintEvent.MemoryFull:
                    Console.WriteLine("Memoria sensore esaurita");
                    break;
                case FingerPrintEvent.FingerPrintDuplicate:
                    Console.WriteLine("Impronta già presente in memoria");
                    break;
                case FingerPrintEvent.OperationCancelled:
                    Console.WriteLine("Operazione annullata");
                    break;
            }
        }
    }
}
