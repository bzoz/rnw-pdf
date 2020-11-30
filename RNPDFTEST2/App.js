/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';
import {
  SafeAreaView,
  StyleSheet,
  ScrollView,
  View,
  Text,
  StatusBar,
  requireNativeComponent,  Button,
  findNodeHandle,
  UIManager
} from 'react-native';

const RCTPdf = requireNativeComponent('RCTPdf');

const App: () => React$Node = () => {
  let pdfRef;
  function onPress() {
    if (pdfRef) {
      const tag = findNodeHandle(pdfRef)
      UIManager.dispatchViewManagerCommand(tag, UIManager.getViewManagerConfig('RCTPdf').Commands.setPage, [10]);
    }
  }
  return (
    <>
      <Button onPress={onPress} title="Go To Page 10" />
      <RCTPdf
        path="ms-appx:///TestPDF.pdf"
        page={1006}
        onError={(event) => { console.log(`Error: ${event.nativeEvent.error}`) }}
        onLoadComplete={(event) => { console.log(`Pages: ${event.nativeEvent.totalPages}, 1st page size: ${event.nativeEvent.width}x${event.nativeEvent.height}`) }}
        onPageChanged={(event) => { console.log(`Page: ${event.nativeEvent.page}/${event.nativeEvent.totalPages}`) }}
        onScaleChanged={(event) => { console.log(`New sacale: ${event.nativeEvent.scale}`) }}
        style={{ width: 800, height: 900 }}
        ref={(ref) => { pdfRef = ref }}
      />
    </>
  );
};

export default App;
