// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_BACKEND_WIN_HELPER_H_
#define PRINTING_BACKEND_WIN_HELPER_H_

#include <objidl.h>
#include <winspool.h>
#include <prntvpt.h>
#include <xpsprint.h>

#include <string>

#include "base/string16.h"
#include "base/win/scoped_handle.h"
#include "printing/printing_export.h"

// These are helper functions for dealing with Windows Printing.
namespace printing {

struct PRINTING_EXPORT PrinterBasicInfo;

class PrinterHandleTraits {
 public:
  typedef HANDLE Handle;

  static bool CloseHandle(HANDLE handle) {
    return ::ClosePrinter(handle) != FALSE;
  }

  static bool IsHandleValid(HANDLE handle) {
    return handle != NULL;
  }

  static HANDLE NullHandle() {
    return NULL;
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PrinterHandleTraits);
};

class ScopedPrinterHandle
    : public base::win::GenericScopedHandle<PrinterHandleTraits,
                                            base::win::VerifierTraits> {
 public:
  bool OpenPrinter(const wchar_t* printer) {
    // ::OpenPrinter may return error but assign some value into handle.
    if (!::OpenPrinter(const_cast<LPTSTR>(printer), Receive(), NULL)) {
      Take();
    }
    return IsValid();
  }

 private:
  typedef base::win::GenericScopedHandle<PrinterHandleTraits,
                                         base::win::VerifierTraits> Base;
  // Hide Receive to avoid assigning handle when ::OpenPrinter returned error.
  Base::Receiver Receive() {
    return Base::Receive();
  }
};

// Wrapper class to wrap the XPS APIs (PTxxx APIs). This is needed because these
// APIs are not available by default on XP. We could delayload prntvpt.dll but
// this would mean having to add that to every binary that links with
// printing.lib (which is a LOT of binaries). So choosing the GetProcAddress
// route instead).
class PRINTING_EXPORT XPSModule {
 public:
  // All the other methods can ONLY be called after a successful call to Init.
  // Init can be called many times and by multiple threads.
  static bool Init();
  static HRESULT OpenProvider(const string16& printer_name,
                              DWORD version,
                              HPTPROVIDER* provider);
  static HRESULT GetPrintCapabilities(HPTPROVIDER provider,
                                      IStream* print_ticket,
                                      IStream* capabilities,
                                      BSTR* error_message);
  static HRESULT ConvertDevModeToPrintTicket(HPTPROVIDER provider,
                                             ULONG devmode_size_in_bytes,
                                             PDEVMODE devmode,
                                             EPrintTicketScope scope,
                                             IStream* print_ticket);
  static HRESULT ConvertPrintTicketToDevMode(
      HPTPROVIDER provider,
      IStream* print_ticket,
      EDefaultDevmodeType base_devmode_type,
      EPrintTicketScope scope,
      ULONG* devmode_byte_count,
      PDEVMODE* devmode,
      BSTR* error_message);
  static HRESULT MergeAndValidatePrintTicket(HPTPROVIDER provider,
                                             IStream* base_ticket,
                                             IStream* delta_ticket,
                                             EPrintTicketScope scope,
                                             IStream* result_ticket,
                                             BSTR* error_message);
  static HRESULT ReleaseMemory(PVOID buffer);
  static HRESULT CloseProvider(HPTPROVIDER provider);

 private:
  XPSModule() { }
  static bool InitImpl();
};

// See comments in cc file explaining why we need this.
class PRINTING_EXPORT ScopedXPSInitializer {
 public:
  ScopedXPSInitializer();
  ~ScopedXPSInitializer();

  bool initialized() const { return initialized_; }

 private:
  bool initialized_;
};

// Wrapper class to wrap the XPS Print APIs (these are different from the PTxxx
// which deal with the XML Print Schema). This is needed because these
// APIs are only available on Windows 7 and higher.
class PRINTING_EXPORT XPSPrintModule {
 public:
  // All the other methods can ONLY be called after a successful call to Init.
  // Init can be called many times and by multiple threads.
  static bool Init();
  static HRESULT StartXpsPrintJob(
      const LPCWSTR printer_name,
      const LPCWSTR job_name,
      const LPCWSTR output_file_name,
      HANDLE progress_event,
      HANDLE completion_event,
      UINT8* printable_pages_on,
      UINT32 printable_pages_on_count,
      IXpsPrintJob **xps_print_job,
      IXpsPrintJobStream **document_stream,
      IXpsPrintJobStream **print_ticket_stream);
 private:
  XPSPrintModule() { }
  static bool InitImpl();
};

PRINTING_EXPORT bool InitBasicPrinterInfo(HANDLE printer,
                                          PrinterBasicInfo* printer_info);

PRINTING_EXPORT std::string GetDriverInfo(HANDLE printer);

}  // namespace printing

#endif  // PRINTING_BACKEND_WIN_HELPER_H_
