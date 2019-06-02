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


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int m, int delta, std::ofstream& ofs2, const std::vector<char>& embed) {  // luminanceにはmフレームの輝度値が含まれている
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
	int flg;
	int vec_count = 0;
	int sum_em[4] = {};
	float temp2 = 0;
	float average_temp = 0;
	std::vector<std::vector<int>> sum_all(4, std::vector<int>());

	for (i = 0; i < BG_width * BG_height; i++) { // フレームのすべてのブロックについて標準偏差がδより小さいものの個数をカウントする．
		//n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));    // vecの中で，deltaより値が小さいものの個数をカウント
		for (auto itr = vec[i].begin(); itr != vec[i].end(); itr++) {    // 225回のループ
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
				else {          // 正解？
					sum_em[1]++;
				}
			}
			else {
				if (flg > 0) {   // 正解?
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

		threshold += n_count[i];    // その個数を閾値に加える．
	}

	// 参考資料のためのグラフを作るための関数
	make_graph(ofs2, n_count, sum_all);

	threshold = threshold / (BG_width * BG_height);  // それをブロック群個数で割る．平均化する
													 // 閾値求め終わり

	for (i = 0; i < BG_width * BG_height; i++) {  // どれだけδ未満の画素値のものがあったか判断する．閾値以上であれば0で復号
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
//	// frame1からframe2にかけて動きベクトルを求める，それをmotion_vecに格納する
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
//	// 復号ビット決定関数	(現状各ビットの多数決で復号ビットを決定する(でもそれだとnum_embedframe個の候補ができてしまってどれを選べばよいかわからない))
//	// num_embedframe個に分けて格納されたall_bit_array配列のそれぞれのm個の復号ビット列に対する処理である
//	std::vector<std::vector<int>> temp_result;
//
//	for (int k = 0; k < num_embedframe; k++) {       // num_embedframeに分けられているから
//		for (int i = 0; i < DECODER_BIT; i++) {      // 透かしビット144それぞれに
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
//	// c_dec_bitの復号ビットをdec_bit_arrayの適切なところへ格納する
//	int cset = cframe / num_embedframe; // mセットのうち，現在のセット番目を格納する
//	int no_cset;  //　現在のセットの中で何番目に格納されるか
//
//	for (int i = 0; i < end(all_bit_array[cset]) - begin(all_bit_array[cset]); i++) {
//		if (all_bit_array[cset][i][0] == (char)(-1)) {
//			no_cset = i;
//			break;
//		}
//	}
//
//	for (int i = 0; i < DECODER_BIT; i++) {       // 本当はvectorの関数を用いて一気に格納したい
//		all_bit_array[cset][no_cset].push_back(c_dec_bit[i]);
//	}
//}
