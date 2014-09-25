#!/bin/bash

case "$1" in 
    "0")
        ./nachos -d ts -F batch0 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "1")
        ./nachos -d ts -F batch1 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "2")
        ./nachos -d ts -F batch2 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "3")
        ./nachos -d ts -F batch3 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "4")
        ./nachos -d ts -F batch4 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "5")
        ./nachos -d ts -F batch5 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "6")
        ./nachos -d ts -F batch6 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "7")
        ./nachos -d ts -F batch7 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "8")
        ./nachos -d ts -F batch8 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "9")
        ./nachos -d ts -F batch9 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
    "10")
        ./nachos -d ts -F batch10 > temp 
        cat temp | tail -22 | head -16
        grep 'Thread' temp | head -11 | tail -10
        ;;
esac
rm -f temp
