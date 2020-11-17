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
      <RCTPdf sampleProp="elo elo" style={{width: 200, height:50}} />
    </>
  );
};

export default App;
