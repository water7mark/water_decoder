#include "decoder.h"


static int delta_thisfile = 100;

void init_decoder(cv::VideoCapture* cap, std::vector<char>* embed, cv::Size* size, std::ofstream* ofs, std::string read_file, std::string write_file, int m, int delta) {
	*embed = set_embeddata(embedfile);
	*cap = capture_open(read_file);
	size->width = cap->get(CV_CAP_PROP_FRAME_WIDTH);
	size->height = cap->get(CV_CAP_PROP_FRAME_HEIGHT);

	ofs->open(write_file);
	*ofs << "動画像 " << read_file << std::endl;
	*ofs << "埋め込み強度δ = " << delta << std::endl;
	*ofs << "ブロックサイズ = " << block_width << "×" << block_height << std::endl;
	*ofs << "連続ブロック数m = " << m << std::endl;
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
			float mean = 0, sum = 0;        // meanはブロック内平均輝度値 ,sumはブロック内合計輝度値
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


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int m, int delta, const std::vector<char>& embed) {  // luminanceにはmフレームの輝度値が含まれている
	std::vector<cv::Mat> means;
	std::vector<cv::Mat> deviations;
	cv::Mat m_means = cv::Mat::zeros(1920, 1080, CV_32F);  // mフレームでの"ブロックごとの輝度平均値"の平均値
	std::vector<cv::Mat> m_deviations;
	int i, j, x, y;


	for (i = 0; i < m; i++) {
		means.push_back(filter(luminance[i]));          // ブロックごとに均した輝度値配列をmeansに格納する．
		luminance[i].convertTo(luminance[i], CV_32F);
		deviations.push_back(luminance[i] - means[i]);  // 各画素輝度値-"ブロックごとの平均輝度値"の結果を偏差(deviations)に格納する
	}

	m_means = means[0].clone() / m;
	for (i = 1; i < m; i++) {
		m_means += means[i] / m;
	}

	for (i = 0; i < m; i++) {
		m_deviations.push_back(means[i] - m_means);    // ブロックごとの平均輝度値 - mフレーム間での"ブロックごとの平均輝度値"の平均値
	}


	std::vector<std::vector<float>> vec;
	vec.resize(BG_width * BG_height);         // vecのサイズ変更, vecはブロック群の中のブロックの個数分のサイズを持っている.

	for (y = 0; y < 1080; y += block_height) {
		for (x = 0; x < 1920; x += block_width) {
			float sum = 0.00;
			for (i = 0; i < m; i++) {          // mフレームでの操作
				sum += m_deviations[i].at<float>(y, x) * m_deviations[i].at<float>(y, x);    // mフレーム分の各画素の輝度値の偏差の2乗和をとる
			}
			vec[(x / block_width) % BG_width + ((y / block_height) % BG_height)*BG_width].push_back(sqrt(sum / m));
			// ↑vecの(x,y)が所属しているブロック群の箇所に平均値の平均値に対する平均値の標準偏差を格納する
		}
	}

	// 閾値を求める
	std::vector<size_t> n_count;
	float threshold = 0;
	delta_thisfile = delta;

	for (i = 0; i < BG_width * BG_height; i++) {
		n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));
		threshold += n_count[i];
	}

	threshold = threshold / (BG_width * BG_height);  // それをブロック群個数で割る．平均化する
													 // 閾値求め終わり

	for (i = 0; i < BG_width * BG_height; i++) {  // どれだけδ未満の画素値のものがあったか判断する．閾値以上であれば0で復号
		if (n_count[i] >= threshold)
			decode->push_back('0');
		else
			decode->push_back('1');
	}
}


