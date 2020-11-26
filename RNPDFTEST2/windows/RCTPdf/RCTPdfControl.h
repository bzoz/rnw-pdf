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
      PDFPageInfo(winrt::Windows::UI::Xaml::Controls::Image image, winrt::Windows::Data::Pdf::PdfPage page, double imageScale, double renderScale);
      PDFPageInfo(const PDFPageInfo&) = default;
      PDFPageInfo(PDFPageInfo&&) = default;
      PDFPageInfo& operator=(const PDFPageInfo&) = default;
      PDFPageInfo& operator=(PDFPageInfo&&) = default;
      double pageVisiblePixels(bool horizontal, double viewportStart, double viewportEnd) const;
      double pageSize(bool horizontal) const;
      double height, width;
      double scaledHeight, scaledWidth;
      double scaledTopOffset, scaledLeftOffset;
      double imageScale, renderScale;
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

        // Global lock to access the data stuff
        // the pages are rendered in an async way
        std::mutex m_mutex;
        // "load index" - increse this every time we load a file so the page renderer task
        // knows if its data is stil up to date
        unsigned m_pdfLoadedIndex = 0;
        // URI and password of the PDF
        std::string m_pdfURI;
        std::string m_pdfPassword;
        // Current active page
        int m_currentPage = 0;
        // Margins of each page
        int m_margins = 10;
        // Scale at which the PDF is displayed
        double m_scale = 1;
        // Are we in "horizontal" mode?
        bool m_horizontal = false;
        // Render the pages in reverse order
        bool m_reverse = false;

        std::vector<PDFPageInfo> m_pages;
        void UpdatePagesInfoMarginOrScale();

        void OnViewChanged(winrt::Windows::Foundation::IInspectable const& sender,
          winrt::Windows::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs const& args);
        winrt::Windows::UI::Xaml::Controls::ScrollViewer::ViewChanged_revoker m_viewChangedRevoker{};

        winrt::fire_and_forget LoadPDF(std::unique_lock<std::mutex> lock);
        winrt::IAsyncAction UpadtePageRender(int page);
        winrt::fire_and_forget UpdateMultiplePagesRender(int firstPage, int endPage);
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
