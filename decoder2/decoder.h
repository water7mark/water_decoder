#pragma once

#ifndef _DECODER_H
#define _DECODER_H


#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <fstream>
#include <string> 


/* ���̃v���O�����́C�������̕������s�����̂ł��D
�o�͂́C�擪�t���[������m�t���[�����������s�����ۂ̕������ʂ���ѕ�������csv�t�@�C���ɏo�͂��܂��D
���̌�Ctxt�t�@�C���ɏ�L�̕������ʂ𓥂܂��đS�̂Ƃ��Č��肵�������r�b�g���o�͂��܂��D

/*Parameter*/
//GUI�œ��͂���\��

#define PROJECT_LOOP 5   // �v���O�����̎��s��
#define DECODER_BIT 144       // ���������ߍ��݃r�b�g��
//#define DEC_RATE_THRESHOLD 80 // �����r�b�g���莞�Ɏg�p����臒l



const std::string embedfile("C:/Users/youhei/Desktop/research_all/research_data/m1_try00_embednum16x9.txt");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::string basis_read_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/ver1/ver1_1/xxx_ver1_1.mp4");
const std::string basis_write_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/test/xxx_ver1_1.csv");
const int m = 20;
const int delta = 1;
const int  block_width = 8;//1�u���b�N�̉���
const int block_height = 8;//1�u���b�N�̏c��

const int  BG_width = 16; //1�u���b�N�Q�̃u���b�N���i���j
const int BG_height = 9; //1�u���b�N�Q�̃u���b�N���i�c�j

const std::string write_file_ex("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/make_data/make_data_xxx_ver1_1_mponly.csv");


// add
const std::string log_file("C:/Users/youhei/Desktop/research_all/research_data/log_all.txt");
void log_write(std::string read_file, std::string write_file);   // �ǂ̃v���O�������ǂ̈����ł������������L�^���邽�߂̊֐�
void str_checker(std::string read_file, std::string write_file); // �������Ԉ���Ă��Ȃ������ׂ�֐�


//�֐��v���g�^�C�v�錾
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