#!/bin/bash

curl -v -X POST --data-binary @1mb.test http://127.0.0.1:8080/upload/final_test.bin
