/*
 * main.cpp
 *
 *  Created on: May 11, 2019
 *      Author: fly
 */
#include "AudioConvert.h"
#include <iostream>

using namespace std;
int main(){
	AudioConvert convertor("1.flac", "2.aac", FORMAT_ID_AAC, 96000, 2);

	convertor.convert(NULL, NULL);
//	cout << AudioConvert::getDefaultOutputFile("hjy.MP3", FORMAT_ID_AAC) << endl;
//	cout << AudioConvert::getDefaultOutputFile("wzq.aac", FORMAT_ID_MP3) << endl;
}



