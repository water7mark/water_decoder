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

#define PROJECT_LOOP 1   // �v���O�����̎��s��
#define DECODER_BIT 72       // ���������ߍ��݃r�b�g��
//#define DEC_RATE_THRESHOLD 80 // �����r�b�g���莞�Ɏg�p����臒l



const std::string embedfile("C:/Users/youhei/Desktop/research_all/research_data/test_m2_embed.txt");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//C: / share_ubuntu / embedder_mp4 / me_xxx_d1.mp4
// C:/share_ubuntu/embedder_mp4/daad/daad_2M_me_xxx_d1.mp4
const std::string basis_read_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/motion_vector/test_xxx_d5.avi");
const std::string basis_write_file("c:/users/youhei/desktop/research_all/research_data/mp4_embedded_videos/motion_vector/output/test/test_xxx_d5.csv");
const std::string basis_motion_vector_file("C:/share_ubuntu/output/xxx_ori_mp4_aviutl.csv");

const int num_embedframe = 20;      
const int delta = 5;


const int  block_width = 16;//1�u���b�N�̉���
const int block_height = 16;//1�u���b�N�̏c��

const int block_size = 16;  //  DCT�u���b�N�̃T�C�Y
const int motionvector_block_size = 16; // �����x�N�g���̃O���b�h�u���b�N�̃T�C�Y

const int  BG_width = 12; //1�u���b�N�Q�̃u���b�N���i���j
const int BG_height = 6; //1�u���b�N�Q�̃u���b�N���i�c�j

const int FRAME_width = 1920;
const int FRAME_height = 1080;


class mv_class {    // ���f�[�^���Ƃ肠�����������Ċi�[����p�̃N���X
public:
	int frame_index;   // �t���[���̔ԍ�
	cv::Mat x_vector;
	cv::Mat y_vector;
};


// add
const std::string log_file("C:/Users/youhei/Desktop/research_all/research_data/log_all.txt");
void log_write(std::string read_file, std::string write_file);   // �ǂ̃v���O�������ǂ̈����ł������������L�^���邽�߂̊֐�
void str_checker(std::string read_file, std::string write_file); // �������Ԉ���Ă��Ȃ������ׂ�֐�


//�֐��v���g�^�C�v�錾
void init_decoder(cv::VideoCapture* cap, std::vector<char>* embed, cv::Size* size, std::ofstream* ofs, std::string read_file, std::string write_file, int m, int delta);
std::vector<char> set_embeddata(const std::string filename);
cv::VideoCapture capture_open(const std::string read_file);
cv::Mat filter(cv::Mat luminance);
void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int delta, const std::vector<char>& embed, int cframe, std::string motion_vector_file);
bool is_less_than(float i);

std::pair<int, int > get_back_pos(std::vector<mv_class>& mv_all, int frame, int y, int x);
bool Is_there_mv(std::vector<mv_class> &mv_all, int frame);

//common

bool overwrite_check(std::string write_file);
void change_filename(std::string& read_file, std::string& write_file, int loop_count, std::string &motion_vector_file);

#endif