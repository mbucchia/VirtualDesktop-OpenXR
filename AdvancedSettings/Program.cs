﻿// MIT License
//
// Copyright(c) 2022-2024 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using System.Security.Principal;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AdvancedSettings
{
    internal static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
#if !DEBUG
            var principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
            if (!principal.IsInRole(WindowsBuiltInRole.Administrator))
            {
                var processInfo = new System.Diagnostics.ProcessStartInfo();
                processInfo.Verb = "RunAs";
                processInfo.FileName = Assembly.GetEntryAssembly().Location;
                SetArguments(processInfo, args);
                try
                {
                    Process.Start(processInfo).WaitForExit();
                }
                catch (Win32Exception)
                {
                    MessageBox.Show("This application must be run as Administrator.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                Application.Exit();
                return;
            }
#endif
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            var regPrefix = "SOFTWARE\\Virtual Desktop, Inc.\\OpenXR";
            if (args.Length >= 1 && args[0] == "/standalone")
            {
                regPrefix = "SOFTWARE\\VirtualDesktop-OpenXR";
            }
            Application.Run(new MainForm(regPrefix, "VDXR"));
        }

        public static void SetArguments(this ProcessStartInfo processStartInfo, IEnumerable<string> args)
        {
            processStartInfo.Arguments = String.Join(" ", args.Select(arg => {
                if (arg.Contains('"')) arg = arg.Replace("\"", "\"\"");
                if (arg.Contains(' ')) arg = '"' + arg + '"';
                return arg;
            }));
        }
    }
}
