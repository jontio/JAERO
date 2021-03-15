#!/bin/bash
#remove what gets installed when building
sudo dpkg --remove libacars-dev libcorrect-dev libaeroambe-dev jaero 
sudo ldconfig
