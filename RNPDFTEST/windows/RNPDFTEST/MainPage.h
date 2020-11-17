#pragma once
#include "MainPage.g.h"
#include <winrt/Microsoft.ReactNative.h>


namespace winrt::RNPDFTEST::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
    };
}

namespace winrt::RNPDFTEST::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}


