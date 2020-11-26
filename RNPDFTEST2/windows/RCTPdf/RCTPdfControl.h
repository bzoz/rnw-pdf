#pragma once

#include <vector>
#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.ReactNative.h"
#include "NativeModules.h"
#include "RCTPdfControl.g.h"

namespace winrt::RCTPdf::implementation
{
    struct PDFPageInfo {
      PDFPageInfo(winrt::Windows::UI::Xaml::Controls::Image image, winrt::Windows::Data::Pdf::PdfPage page, double scale, double margin);
      PDFPageInfo(const PDFPageInfo&) = default;
      PDFPageInfo(PDFPageInfo&&) = default;
      PDFPageInfo& operator=(const PDFPageInfo&) = default;
      PDFPageInfo& operator=(PDFPageInfo&&) = default;
      double height, width;
      double scaledHeight, scaledWidth;
      double scaledTopOffset, scaledLeftOffset;
      double renderedScale;
      winrt::Windows::UI::Xaml::Controls::Image image;
      winrt::Windows::Data::Pdf::PdfPage page;
    };
    struct RCTPdfControl : RCTPdfControlT<RCTPdfControl>
    {
    public:
        RCTPdfControl(Microsoft::ReactNative::IReactContext const& reactContext);

        static winrt::Windows::Foundation::Collections::
          IMapView<winrt::hstring, winrt::Microsoft::ReactNative::ViewManagerPropertyType>
          NativeProps() noexcept;
        void UpdateProperties(winrt::Microsoft::ReactNative::IJSValueReader const& propertyMapReader) noexcept;

        static winrt::Microsoft::ReactNative::ConstantProviderDelegate
          ExportedCustomBubblingEventTypeConstants() noexcept;
        static winrt::Microsoft::ReactNative::ConstantProviderDelegate
          ExportedCustomDirectEventTypeConstants() noexcept;

        static winrt::Windows::Foundation::Collections::IVectorView<winrt::hstring> Commands() noexcept;
        void DispatchCommand(
          winrt::hstring const& commandId,
          winrt::Microsoft::ReactNative::IJSValueReader const& commandArgsReader) noexcept;
    private:
        Microsoft::ReactNative::IReactContext m_reactContext{ nullptr };

        std::mutex m_mutex;
        winrt::Windows::Data::Pdf::PdfDocument m_document;
        std::string m_pdfURI;
        std::string m_pdfPassword;
        int m_currentPage = 0;
        int m_targetOffset = -1;
        int m_margins = 10;
        double m_scale = 1;
        bool m_horizontal = false;

        std::vector<PDFPageInfo> m_pages;
       
        void OnViewChanged(winrt::Windows::Foundation::IInspectable const& sender,
          winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const& args);
        winrt::Windows::UI::Xaml::Controls::ScrollViewer::ViewChanged_revoker m_viewChangedRevoker{};

        winrt::fire_and_forget LoadPDF(std::unique_lock<std::mutex> lock);
        winrt::fire_and_forget UpadtePage(int page);
        void GoToPage(int page);
        void SignalError(const std::string& error);
    };
}

namespace winrt::RCTPdf::factory_implementation
{
    struct RCTPdfControl : RCTPdfControlT<RCTPdfControl, implementation::RCTPdfControl>
    {
    };
}
