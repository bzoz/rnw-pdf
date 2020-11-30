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
  requireNativeComponent
} from 'react-native';

const RCTPdf = requireNativeComponent('RCTPdf');

const App: () => React$Node = () => {
  return (
    <>
      <Text>Hello!</Text>
      <Text>Hello!</Text>
      <Text>Hello!</Text>
      <Text>Hello!</Text>
      <RCTPdf
        path="ms-appx:///TestPDF.pdf"
        page={1006}
        onError={(event) => { console.log(`Error: ${event.nativeEvent.error}`) }}
        onLoadComplete={(event) => { console.log(`Pages: ${event.nativeEvent.totalPages}, 1st page size: ${event.nativeEvent.width}x${event.nativeEvent.height}`) }}
        onPageChanged={(event) => { console.log(`Page: ${event.nativeEvent.page}/${event.nativeEvent.totalPages}`) }}
        onScaleChanged={(event) => { console.log(`New sacale: ${event.nativeEvent.scale}`) }}
        style={{ width: 800, height: 900 }}
      />
    </>
  );
};

export default App;
