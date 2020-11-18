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
      <RCTPdf path="C:\Users\ja\Desktop\doc2.pdf" style={{width: 800, height:900}} />
    </>
  );
};

export default App;