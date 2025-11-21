// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
import { getDatabase } from "firebase/database";
// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyA8L75LfPyMQQIGtfF9xGgrTQELg36AfWE",
  authDomain: "arduino-alarme-d3524.firebaseapp.com",
  databaseURL: "https://arduino-alarme-d3524-default-rtdb.firebaseio.com",
  projectId: "arduino-alarme-d3524",
  storageBucket: "arduino-alarme-d3524.firebasestorage.app",
  messagingSenderId: "959775524746",
  appId: "1:959775524746:web:997625d0ef2ac51709ea73"
};

export const app = initializeApp(firebaseConfig);

export const db = getDatabase(app);