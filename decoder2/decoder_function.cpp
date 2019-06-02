#include "decoder.h"


static int delta_thisfile = 100;

void init_decoder(cv::VideoCapture* cap, std::vector<char>* embed, cv::Size* size, std::ofstream* ofs, std::string read_file, std::string write_file, int m, int delta) {
	*embed = set_embeddata(embedfile);
	*cap = capture_open(read_file);
	size->width = cap->get(CV_CAP_PROP_FRAME_WIDTH);
	size->height = cap->get(CV_CAP_PROP_FRAME_HEIGHT);

	ofs->open(write_file);
	*ofs << "���摜 " << read_file << std::endl;
	*ofs << "���ߍ��݋��x�� = " << delta << std::endl;
	*ofs << "�u���b�N�T�C�Y = " << block_width << "�~" << block_height << std::endl;
	*ofs << "�A���u���b�N��m = " << m << std::endl;
	*ofs << "calculation time[ms]" << "," << "correct" << "," << "error" << ", " << "first frame" << "," << "correct rate" << "," << std::endl;
}

std::vector<char> set_embeddata(const std::string filename) {
	std::vector<char> embed;
	std::ifstream ifs;
	ifs.open(filename);
	if (ifs.fail()) {
		std::cerr << "can't open embed data file\n";
		std::getchar();
		exit(3);
	}

	while (!ifs.eof())
		embed.push_back(ifs.get());
	return embed;
}

cv::VideoCapture capture_open(const std::string read_file) {
	cv::VideoCapture cap(read_file);
	if (!cap.isOpened()) {
		std::cout << "can't open video file.\n";
		std::getchar();
		exit(4);
	}
	return cap;

}

cv::Mat filter(cv::Mat luminance) {
	cv::Mat dst_luminance(luminance.size(), CV_32F);
	int x = 0, y = 0, m = 0, n = 0;
	for (m = 0; m < luminance.cols; m += block_width) {
		for (n = 0; n < luminance.rows; n += block_height) {
			float mean = 0, sum = 0;        // mean�̓u���b�N�����ϋP�x�l ,sum�̓u���b�N�����v�P�x�l
			for (x = m; x < m + block_width; x++) {
				for (y = n; y < n + block_height; y++) {
					sum += (float)luminance.at<uchar>(y, x);
				}
			}

			mean = sum / (block_width * block_height);
			for (x = m; x < m + block_width; x++) {
				for (y = n; y < n + block_height; y++) {
					dst_luminance.at<float>(y, x) = mean;
				}
			}
		}
	}

	return dst_luminance;
}

bool is_less_than(float i) {

	return ((i < delta_thisfile) == 1);
}


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int m, int delta, std::ofstream& ofs2, const std::vector<char>& embed) {  // luminance�ɂ�m�t���[���̋P�x�l���܂܂�Ă���
	std::vector<cv::Mat> means;
	std::vector<cv::Mat> deviations;
	cv::Mat m_means = cv::Mat::zeros(1920, 1080, CV_32F);  // m�t���[���ł�"�u���b�N���Ƃ̋P�x���ϒl"�̕��ϒl
	std::vector<cv::Mat> m_deviations;
	int i, j, x, y;


	for (i = 0; i < m; i++) {
		means.push_back(filter(luminance[i]));          // �u���b�N���Ƃɋς����P�x�l�z���means�Ɋi�[����D
		luminance[i].convertTo(luminance[i], CV_32F);
		deviations.push_back(luminance[i] - means[i]);  // �e��f�P�x�l-"�u���b�N���Ƃ̕��ϋP�x�l"�̌��ʂ�΍�(deviations)�Ɋi�[����
	}

	m_means = means[0].clone() / m;
	for (i = 1; i < m; i++) {
		m_means += means[i] / m;
	}

	for (i = 0; i < m; i++) {
		m_deviations.push_back(means[i] - m_means);    // �u���b�N���Ƃ̕��ϋP�x�l - m�t���[���Ԃł�"�u���b�N���Ƃ̕��ϋP�x�l"�̕��ϒl
	}


	std::vector<std::vector<float>> vec;
	vec.resize(BG_width * BG_height);         // vec�̃T�C�Y�ύX, vec�̓u���b�N�Q�̒��̃u���b�N�̌����̃T�C�Y�������Ă���.

	for (y = 0; y < 1080; y += block_height) {
		for (x = 0; x < 1920; x += block_width) {
			float sum = 0.00;
			for (i = 0; i < m; i++) {          // m�t���[���ł̑���
				sum += m_deviations[i].at<float>(y, x) * m_deviations[i].at<float>(y, x);    // m�t���[�����̊e��f�̋P�x�l�̕΍���2��a���Ƃ�
			}
			vec[(x / block_width) % BG_width + ((y / block_height) % BG_height)*BG_width].push_back(sqrt(sum / m));
			// ��vec��(x,y)���������Ă���u���b�N�Q�̉ӏ��ɕ��ϒl�̕��ϒl�ɑ΂��镽�ϒl�̕W���΍����i�[����
		}
	}

	// 臒l�����߂�
	std::vector<size_t> n_count;
	float threshold = 0;
	delta_thisfile = delta;
	int flg;
	int vec_count = 0;
	int sum_em[4] = {};
	float temp2 = 0;
	float average_temp = 0;
	std::vector<std::vector<int>> sum_all(4, std::vector<int>());

	for (i = 0; i < BG_width * BG_height; i++) { // �t���[���̂��ׂẴu���b�N�ɂ��ĕW���΍�����菬�������̂̌����J�E���g����D
		//n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));    // vec�̒��ŁCdelta���l�����������̂̌����J�E���g
		for (auto itr = vec[i].begin(); itr != vec[i].end(); itr++) {    // 225��̃��[�v
			if (*itr - delta >= 0) {
				flg = 1;
			}
			else {
				flg = -1;
			}

			if (embed[i] == '0') {
				if (flg > 0) {
					sum_em[0]++;
				}
				else {          // �����H
					sum_em[1]++;
				}
			}
			else {
				if (flg > 0) {   // ����?
					sum_em[2]++;
				}
				else {
					sum_em[3]++;
				}
			}

			if (flg < 0) {
				vec_count++;
			}
		}

		n_count.push_back(vec_count);
		vec_count = 0;

		for (int i = 0; i < 4; i++) {
			sum_all[i].push_back(sum_em[i]);
			sum_em[i] = 0;
		}

		threshold += n_count[i];    // ���̌���臒l�ɉ�����D
	}

	// �Q�l�����̂��߂̃O���t����邽�߂̊֐�
	make_graph(ofs2, n_count, sum_all);

	threshold = threshold / (BG_width * BG_height);  // ������u���b�N�Q���Ŋ���D���ω�����
													 // 臒l���ߏI���

	for (i = 0; i < BG_width * BG_height; i++) {  // �ǂꂾ�������̉�f�l�̂��̂������������f����D臒l�ȏ�ł����0�ŕ���
		if (n_count[i] >= threshold)
			decode->push_back('0');
		else
			decode->push_back('1');
	}
}


void make_graph(std::ofstream& ofs2, std::vector<size_t>& n_count, std::vector<std::vector<int>>(&sum_all)) {

	for (int i = 0; i < n_count.end() - n_count.begin(); i++) {
		ofs2 << n_count[i] << ",";


		for (char j = 0; j < 4; j++) {
			ofs2 << sum_all[j][i] << ",";
		}

	}



	ofs2 << std::endl;

}

//void block_matching(cv::Mat frame1, cv::Mat frame2,  int block_size, cv::Mat motion_vec) {
//	// frame1����frame2�ɂ����ē����x�N�g�������߂�C�����motion_vec�Ɋi�[����
//	// 
//
//	for (int x = 0; x < 1920; x += block_width) {
//		for (int y = 0; y < 1080; y += block_height) {
//			frame1.at<unsigned char>(y, x)
//		}
//	}
//
//
//}

//void same_checker(cv::Mat frame, int x, int y) {
//	int bc_width = 0;
//	int bc_height = 0;
//
//	for (int i = x - bc_width + 1; i <= x + bc_width ; i++) {
//		for (int j = y - bc_height + 1; j <= y + bc_width; i++) {
//			frame.at<unsigned char>(j, i)
//		}
//	}
//
//}

//void decide_bit(std::vector<std::vector<std::vector<int>>> all_bit_array, int& result_bit_array, int m, int num_embedframe, int start_frame, int m_set) {
//	// �����r�b�g����֐�	(����e�r�b�g�̑������ŕ����r�b�g�����肷��(�ł����ꂾ��num_embedframe�̌�₪�ł��Ă��܂��Ăǂ��I�ׂ΂悢���킩��Ȃ�))
//	// num_embedframe�ɕ����Ċi�[���ꂽall_bit_array�z��̂��ꂼ���m�̕����r�b�g��ɑ΂��鏈���ł���
//	std::vector<std::vector<int>> temp_result;
//
//	for (int k = 0; k < num_embedframe; k++) {       // num_embedframe�ɕ������Ă��邩��
//		for (int i = 0; i < DECODER_BIT; i++) {      // �������r�b�g144���ꂼ���
//			int mean = 0;
//			for (int j = 0; j < end(all_bit_array[0]) - begin(all_bit_array[0]); j++) {
//				mean += all_bit_array[k][j][i];
//			}
//			mean /= (end(all_bit_array[0]) - begin(all_bit_array[0]));
//		}
//
//	}
//}

//void assort(std::vector<char> c_dec_bit, std::vector<std::vector<std::vector<char>>> all_bit_array, int num_embedframe, int cframe, cv::mat dec_bit_array_1) {
//	// c_dec_bit�̕����r�b�g��dec_bit_array�̓K�؂ȂƂ���֊i�[����
//	int cset = cframe / num_embedframe; // m�Z�b�g�̂����C���݂̃Z�b�g�Ԗڂ��i�[����
//	int no_cset;  //�@���݂̃Z�b�g�̒��ŉ��ԖڂɊi�[����邩
//
//	for (int i = 0; i < end(all_bit_array[cset]) - begin(all_bit_array[cset]); i++) {
//		if (all_bit_array[cset][i][0] == (char)(-1)) {
//			no_cset = i;
//			break;
//		}
//	}
//
//	for (int i = 0; i < DECODER_BIT; i++) {       // �{����vector�̊֐���p���Ĉ�C�Ɋi�[������
//		all_bit_array[cset][no_cset].push_back(c_dec_bit[i]);
//	}
//}
