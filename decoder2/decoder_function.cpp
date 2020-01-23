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


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int m, int delta, const std::vector<char>& embed) {  // luminance�ɂ�m�t���[���̋P�x�l���܂܂�Ă���
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

	for (i = 0; i < BG_width * BG_height; i++) {
		n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));
		threshold += n_count[i];
	}

	threshold = threshold / (BG_width * BG_height);  // ������u���b�N�Q���Ŋ���D���ω�����
													 // 臒l���ߏI���

	for (i = 0; i < BG_width * BG_height; i++) {  // �ǂꂾ�������̉�f�l�̂��̂������������f����D臒l�ȏ�ł����0�ŕ���
		if (n_count[i] >= threshold)
			decode->push_back('0');
		else
			decode->push_back('1');
	}
}


