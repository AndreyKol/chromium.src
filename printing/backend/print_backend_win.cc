// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/backend/print_backend.h"

#include <objidl.h>
#include <winspool.h>

#include "base/memory/scoped_ptr.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_hglobal.h"
#include "printing/backend/print_backend_consts.h"
#include "printing/backend/printing_info_win.h"
#include "printing/backend/win_helper.h"

namespace printing {

namespace {

HRESULT StreamOnHGlobalToString(IStream* stream, std::string* out) {
  DCHECK(stream);
  DCHECK(out);
  HGLOBAL hdata = NULL;
  HRESULT hr = GetHGlobalFromStream(stream, &hdata);
  if (SUCCEEDED(hr)) {
    DCHECK(hdata);
    base::win::ScopedHGlobal<char*> locked_data(hdata);
    out->assign(locked_data.release(), locked_data.Size());
  }
  return hr;
}

template <class T>
void GetDeviceCapabilityArray(const wchar_t* printer,
                              const wchar_t* port,
                              WORD id,
                              std::vector<T>* result) {
  int count = DeviceCapabilities(printer, port, id, NULL, NULL);
  if (count <= 0)
    return;
  std::vector<T> tmp;
  tmp.resize(count * 2);
  count = DeviceCapabilities(printer, port, id,
                             reinterpret_cast<LPTSTR>(tmp.data()), NULL);
  if (count <= 0)
    return;
  CHECK_LE(count, base::checked_cast<int>(tmp.size()));
  tmp.resize(count);
  result->swap(tmp);
}

void LoadPaper(const wchar_t* printer,
               const wchar_t* port,
               const DEVMODE* devmode,
               PrinterSemanticCapsAndDefaults* caps) {
  static const size_t kToUm = 100;  // Windows uses 0.1mm.
  static const size_t kMaxPaperName = 64;

  struct PaperName {
    wchar_t chars[kMaxPaperName];
  };

  DCHECK_EQ(sizeof(PaperName), sizeof(wchar_t) * kMaxPaperName);

  // Paper
  std::vector<PaperName> names;
  GetDeviceCapabilityArray(printer, port, DC_PAPERNAMES, &names);

  std::vector<POINT> sizes;
  GetDeviceCapabilityArray(printer, port, DC_PAPERSIZE, &sizes);

  std::vector<WORD> ids;
  GetDeviceCapabilityArray(printer, port, DC_PAPERS, &ids);

  DCHECK_EQ(ids.size(), sizes.size());
  DCHECK_EQ(names.size(), sizes.size());

  if (ids.size() != sizes.size())
    ids.clear();
  if (names.size() != sizes.size())
    names.clear();

  for (size_t i = 0; i < sizes.size(); ++i) {
    PrinterSemanticCapsAndDefaults::Paper paper;
    paper.size_um.SetSize(sizes[i].x * kToUm, sizes[i].y * kToUm);
    if (!names.empty()) {
      const wchar_t* name_start = names[i].chars;
      base::string16 tmp_name(name_start, kMaxPaperName);
      // Trim trailing zeros.
      tmp_name = tmp_name.c_str();
      paper.display_name = base::WideToUTF8(tmp_name);
    }
    if (!ids.empty())
      paper.vendor_id = base::UintToString(ids[i]);
    caps->papers.push_back(paper);
  }

  if (devmode) {
    // Copy paper with the same ID as default paper.
    if (devmode->dmFields & DM_PAPERSIZE) {
      for (size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == devmode->dmPaperSize) {
          DCHECK_EQ(ids.size(), caps->papers.size());
          caps->default_paper = caps->papers[i];
          break;
        }
      }
    }

    gfx::Size default_size;
    if (devmode->dmFields & DM_PAPERWIDTH)
      default_size.set_width(devmode->dmPaperWidth * kToUm);
    if (devmode->dmFields & DM_PAPERLENGTH)
      default_size.set_height(devmode->dmPaperLength * kToUm);

    if (!default_size.IsEmpty()) {
      // Reset default paper if |dmPaperWidth| or |dmPaperLength| does not
      // match default paper set by.
      if (default_size != caps->default_paper.size_um)
        caps->default_paper = PrinterSemanticCapsAndDefaults::Paper();
      caps->default_paper.size_um = default_size;
    }
  }
}

void LoadDpi(const wchar_t* printer,
             const wchar_t* port,
             const DEVMODE* devmode,
             PrinterSemanticCapsAndDefaults* caps) {
  std::vector<POINT> dpis;
  GetDeviceCapabilityArray(printer, port, DC_ENUMRESOLUTIONS, &dpis);

  for (size_t i = 0; i < dpis.size() ; ++i)
    caps->dpis.push_back(gfx::Size(dpis[i].x, dpis[i].y));

  if (devmode) {
    if ((devmode->dmFields & DM_PRINTQUALITY) && devmode->dmPrintQuality > 0) {
      caps->default_dpi.SetSize(devmode->dmPrintQuality,
                                devmode->dmPrintQuality);
      if (devmode->dmFields & DM_YRESOLUTION) {
        caps->default_dpi.set_height(devmode->dmYResolution);
      }
    }
  }
}

}  // namespace

class PrintBackendWin : public PrintBackend {
 public:
  PrintBackendWin() {}

  // PrintBackend implementation.
  virtual bool EnumeratePrinters(PrinterList* printer_list) OVERRIDE;
  virtual std::string GetDefaultPrinterName() OVERRIDE;
  virtual bool GetPrinterSemanticCapsAndDefaults(
      const std::string& printer_name,
      PrinterSemanticCapsAndDefaults* printer_info) OVERRIDE;
  virtual bool GetPrinterCapsAndDefaults(
      const std::string& printer_name,
      PrinterCapsAndDefaults* printer_info) OVERRIDE;
  virtual std::string GetPrinterDriverInfo(
      const std::string& printer_name) OVERRIDE;
  virtual bool IsValidPrinter(const std::string& printer_name) OVERRIDE;

 protected:
  virtual ~PrintBackendWin() {}
};

bool PrintBackendWin::EnumeratePrinters(PrinterList* printer_list) {
  DCHECK(printer_list);
  DWORD bytes_needed = 0;
  DWORD count_returned = 0;
  const DWORD kLevel = 4;
  BOOL ret = EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL,
                          kLevel, NULL, 0, &bytes_needed, &count_returned);
  if (!bytes_needed)
    return false;
  scoped_ptr<BYTE[]> printer_info_buffer(new BYTE[bytes_needed]);
  ret = EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS, NULL, kLevel,
                     printer_info_buffer.get(), bytes_needed, &bytes_needed,
                     &count_returned);
  DCHECK(ret);
  if (!ret)
    return false;

  std::string default_printer = GetDefaultPrinterName();
  PRINTER_INFO_4* printer_info =
      reinterpret_cast<PRINTER_INFO_4*>(printer_info_buffer.get());
  for (DWORD index = 0; index < count_returned; index++) {
    ScopedPrinterHandle printer;
    PrinterBasicInfo info;
    if (printer.OpenPrinter(printer_info[index].pPrinterName) &&
        InitBasicPrinterInfo(printer, &info)) {
      info.is_default = (info.printer_name == default_printer);
      printer_list->push_back(info);
    }
  }
  return true;
}

std::string PrintBackendWin::GetDefaultPrinterName() {
  DWORD size = MAX_PATH;
  TCHAR default_printer_name[MAX_PATH];
  if (!::GetDefaultPrinter(default_printer_name, &size))
    return std::string();
  return base::WideToUTF8(default_printer_name);
}

bool PrintBackendWin::GetPrinterSemanticCapsAndDefaults(
    const std::string& printer_name,
    PrinterSemanticCapsAndDefaults* printer_info) {
  ScopedPrinterHandle printer_handle;
  if (!printer_handle.OpenPrinter(base::UTF8ToWide(printer_name).c_str())) {
    LOG(WARNING) << "Failed to open printer, error = " << GetLastError();
    return false;
  }

  PrinterInfo5 info_5;
  if (!info_5.Init(printer_handle))
    return false;
  const wchar_t* name = info_5.get()->pPrinterName;
  const wchar_t* port = info_5.get()->pPortName;
  DCHECK_EQ(name, base::UTF8ToUTF16(printer_name));

  PrinterSemanticCapsAndDefaults caps;

  scoped_ptr<DEVMODE, base::FreeDeleter> user_settings =
      CreateDevMode(printer_handle, NULL);
  if (user_settings) {
    if (user_settings->dmFields & DM_COLOR)
      caps.color_default = (user_settings->dmColor == DMCOLOR_COLOR);

    if (user_settings->dmFields & DM_DUPLEX) {
      switch (user_settings->dmDuplex) {
      case DMDUP_SIMPLEX:
        caps.duplex_default = SIMPLEX;
        break;
      case DMDUP_VERTICAL:
        caps.duplex_default = LONG_EDGE;
        break;
      case DMDUP_HORIZONTAL:
        caps.duplex_default = SHORT_EDGE;
        break;
      default:
        NOTREACHED();
      }
    }

    if (user_settings->dmFields & DM_COLLATE)
      caps.collate_default = (user_settings->dmCollate == DMCOLLATE_TRUE);
  } else {
    LOG(WARNING) << "Fallback to color/simplex mode.";
    caps.color_default = caps.color_changeable;
    caps.duplex_default = SIMPLEX;
  }

  // Get printer capabilities. For more info see here:
  // http://msdn.microsoft.com/en-us/library/windows/desktop/dd183552(v=vs.85).aspx
  caps.color_changeable =
      (DeviceCapabilities(name, port, DC_COLORDEVICE, NULL, NULL) == 1);
  caps.color_model = printing::COLOR;
  caps.bw_model = printing::GRAY;

  caps.duplex_capable =
      (DeviceCapabilities(name, port, DC_DUPLEX, NULL, NULL) == 1);

  caps.collate_capable =
      (DeviceCapabilities(name, port, DC_COLLATE, NULL, NULL) == 1);

  caps.copies_capable =
      (DeviceCapabilities(name, port, DC_COPIES, NULL, NULL) > 1);

  LoadPaper(name, port, user_settings.get(), &caps);
  LoadDpi(name, port, user_settings.get(), &caps);

  *printer_info = caps;
  return true;
}

bool PrintBackendWin::GetPrinterCapsAndDefaults(
    const std::string& printer_name,
    PrinterCapsAndDefaults* printer_info) {
  ScopedXPSInitializer xps_initializer;
  if (!xps_initializer.initialized()) {
    // TODO(sanjeevr): Handle legacy proxy case (with no prntvpt.dll)
    return false;
  }
  if (!IsValidPrinter(printer_name)) {
    return false;
  }
  DCHECK(printer_info);
  HPTPROVIDER provider = NULL;
  std::wstring printer_name_wide = base::UTF8ToWide(printer_name);
  HRESULT hr = XPSModule::OpenProvider(printer_name_wide, 1, &provider);
  if (provider) {
    base::win::ScopedComPtr<IStream> print_capabilities_stream;
    hr = CreateStreamOnHGlobal(NULL, TRUE,
                               print_capabilities_stream.Receive());
    DCHECK(SUCCEEDED(hr));
    if (print_capabilities_stream) {
      base::win::ScopedBstr error;
      hr = XPSModule::GetPrintCapabilities(provider,
                                           NULL,
                                           print_capabilities_stream,
                                           error.Receive());
      DCHECK(SUCCEEDED(hr));
      if (FAILED(hr)) {
        return false;
      }
      hr = StreamOnHGlobalToString(print_capabilities_stream.get(),
                                   &printer_info->printer_capabilities);
      DCHECK(SUCCEEDED(hr));
      printer_info->caps_mime_type = "text/xml";
    }
    ScopedPrinterHandle printer_handle;
    if (printer_handle.OpenPrinter(printer_name_wide.c_str())) {
      scoped_ptr<DEVMODE, base::FreeDeleter> devmode_out(
          CreateDevMode(printer_handle, NULL));
      if (!devmode_out)
        return false;
      base::win::ScopedComPtr<IStream> printer_defaults_stream;
      hr = CreateStreamOnHGlobal(NULL, TRUE,
                                 printer_defaults_stream.Receive());
      DCHECK(SUCCEEDED(hr));
      if (printer_defaults_stream) {
        DWORD dm_size = devmode_out->dmSize + devmode_out->dmDriverExtra;
        hr = XPSModule::ConvertDevModeToPrintTicket(provider, dm_size,
            devmode_out.get(), kPTJobScope, printer_defaults_stream);
        DCHECK(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) {
          hr = StreamOnHGlobalToString(printer_defaults_stream.get(),
                                       &printer_info->printer_defaults);
          DCHECK(SUCCEEDED(hr));
          printer_info->defaults_mime_type = "text/xml";
        }
      }
    }
    XPSModule::CloseProvider(provider);
  }
  return true;
}

// Gets the information about driver for a specific printer.
std::string PrintBackendWin::GetPrinterDriverInfo(
    const std::string& printer_name) {
  ScopedPrinterHandle printer;
  if (!printer.OpenPrinter(base::UTF8ToWide(printer_name).c_str())) {
    return std::string();
  }
  return GetDriverInfo(printer);
}

bool PrintBackendWin::IsValidPrinter(const std::string& printer_name) {
  ScopedPrinterHandle printer_handle;
  return printer_handle.OpenPrinter(base::UTF8ToWide(printer_name).c_str());
}

scoped_refptr<PrintBackend> PrintBackend::CreateInstance(
    const base::DictionaryValue* print_backend_settings) {
  return new PrintBackendWin;
}

}  // namespace printing
