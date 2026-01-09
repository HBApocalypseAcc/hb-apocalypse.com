using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using MessageBox = System.Windows.MessageBox;

namespace ApocalypseLauncher
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private const string LAUNCHER_MUTEX_NAME = "ApocalypseHB_Launcher_Singleton";
        private Mutex? _instanceMutex;

        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        private const int SW_RESTORE = 9;

        protected override void OnStartup(StartupEventArgs e)
        {
            SetupUnhandledExceptionLogging();

            // Instancia única: intenta adquirir el Mutex. Si ya está adquirido por otro proceso,
            // informamos al usuario (evita el "no abre nada").
            bool createdNew;
            _instanceMutex = new Mutex(false, LAUNCHER_MUTEX_NAME, out createdNew);

            bool hasHandle = false;
            try
            {
                hasHandle = _instanceMutex.WaitOne(0, false);
            }
            catch (AbandonedMutexException)
            {
                // Si el launcher anterior crasheó, el mutex queda "abandonado".
                // Lo tratamos como disponible.
                hasHandle = true;
            }

            if (!hasHandle)
            {
                BringExistingInstanceToFront();
                Shutdown(0);
                return;
            }

            base.OnStartup(e);

            MainWindow = new MainWindow();
            MainWindow.Show();
        }

        private static void BringExistingInstanceToFront()
        {
            try
            {
                using var current = Process.GetCurrentProcess();
                string? currentPath = null;
                try
                {
                    currentPath = Environment.ProcessPath;
                }
                catch
                {
                    // ignore
                }

                foreach (var p in Process.GetProcessesByName(current.ProcessName))
                {
                    try
                    {
                        if (p.Id == current.Id)
                        {
                            continue;
                        }

                        // Best-effort: ensure we target the same executable.
                        if (!string.IsNullOrWhiteSpace(currentPath))
                        {
                            try
                            {
                                if (!string.Equals(p.MainModule?.FileName, currentPath, StringComparison.OrdinalIgnoreCase))
                                {
                                    continue;
                                }
                            }
                            catch
                            {
                                // Access denied to MainModule on some setups; fall back to bringing the first match.
                            }
                        }

                        IntPtr hWnd = p.MainWindowHandle;
                        if (hWnd == IntPtr.Zero)
                        {
                            continue;
                        }

                        ShowWindow(hWnd, SW_RESTORE);
                        SetForegroundWindow(hWnd);
                        return;
                    }
                    catch
                    {
                        // ignore this process and continue
                    }
                    finally
                    {
                        try { p.Dispose(); } catch { }
                    }
                }
            }
            catch
            {
                // best-effort
            }
        }

        protected override void OnExit(ExitEventArgs e)
        {
            try
            {
                _instanceMutex?.ReleaseMutex();
            }
            catch
            {
                // Ignorar: puede no ser el owner.
            }
            finally
            {
                _instanceMutex?.Dispose();
                _instanceMutex = null;
            }

            base.OnExit(e);
        }

        private void SetupUnhandledExceptionLogging()
        {
            DispatcherUnhandledException += (_, args) =>
            {
                LogFatal(args.Exception, "DispatcherUnhandledException");
                MessageBox.Show(
                    "El launcher se cerró por un error.\n\nSe guardó un log en LocalAppData (carpeta ApocalypseLauncher).",
                    "Apocalypse Launcher",
                    MessageBoxButton.OK,
                    MessageBoxImage.Error);
                args.Handled = true;
                Shutdown(-1);
            };

            AppDomain.CurrentDomain.UnhandledException += (_, args) =>
            {
                if (args.ExceptionObject is Exception ex)
                {
                    LogFatal(ex, "AppDomain.UnhandledException");
                }
            };

            TaskScheduler.UnobservedTaskException += (_, args) =>
            {
                LogFatal(args.Exception, "TaskScheduler.UnobservedTaskException");
                args.SetObserved();
            };
        }

        private static void LogFatal(Exception ex, string source)
        {
            try
            {
                string logDir = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "ApocalypseLauncher");

                Directory.CreateDirectory(logDir);

                string logPath = Path.Combine(logDir, "launcher_crash.log");
                string entry =
                    $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {source}{Environment.NewLine}" +
                    ex + Environment.NewLine +
                    "------------------------------------------------------------" +
                    Environment.NewLine;

                File.AppendAllText(logPath, entry);
            }
            catch
            {
                // No podemos loguear: no bloquear el cierre.
            }
        }
    }

}
