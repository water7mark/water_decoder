#pragma once

#ifndef _DECODER_H
#define _DECODER_H


#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <string> 


/* このプログラムは，透かしの復号を行うものです．
出力は，先頭フレームからmフレームずつ復号を行った際の復号結果および復号率をcsvファイルに出力します．
その後，txtファイルに上記の復号結果を踏まえて全体として決定した復号ビットを出力します．

/*Parameter*/
//GUIで入力する予定

#define PROJECT_LOOP 5   // プログラムの実行回数
#define DECODER_BIT 144       // 透かし埋め込みビット数
//#define DEC_RATE_THRESHOLD 80 // 復号ビット決定時に使用する閾値



const std::string embedfile("C:/Users/youhei/Desktop/research_all/research_data/m1_try00_embednum16x9.txt");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::string basis_read_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/ver1/ver1_1/xxx_ver1_1.mp4");
const std::string basis_write_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/test/xxx_ver1_1.csv");
const int m = 20;
const int delta = 1;
const int  block_width = 8;//1ブロックの横幅
const int block_height = 8;//1ブロックの縦幅

const int  BG_width = 16; //1ブロック群のブロック個数（横）
const int BG_height = 9; //1ブロック群のブロック個数（縦）

const std::string write_file_ex("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/make_data/make_data_xxx_ver1_1_mponly.csv");


// add
const std::string log_file("C:/Users/youhei/Desktop/research_all/research_data/log_all.txt");
void log_write(std::string read_file, std::string write_file);   // どのプログラムをどの引数でいつ動かしたか記録するための関数
void str_checker(std::string read_file, std::string write_file); // 引数が間違っていないか調べる関数


//関数プロトタイプ宣言
void init_decoder(cv::VideoCapture* cap, std::vector<char>* embed, cv::Size* size, std::ofstream* ofs, std::string read_file, std::string write_file, int m, int delta);
std::vector<char> set_embeddata(const std::string filename);
cv::VideoCapture capture_open(const std::string read_file);
cv::Mat filter(cv::Mat luminance);
void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int m, int delta, std::ofstream& ofs2, const std::vector<char>& embed);
bool is_less_than(float i);
void make_graph(std::ofstream& ofs, std::vector<size_t>& n_count, std::vector<std::vector<int>>(&sumall));


//common

bool overwrite_check(std::string write_file);
void change_filename(std::string& read_file, std::string& write_file, int loop_count, std::string &write_file_ex);

#endif