#include "decoder.h"


static int delta_thisfile = 100;


static int matrix_height = 134;
static int matrix_width = 120;

#define MV_DETA_SIZE CV_8SC1
#define NP_DETA_SIZE CV_16SC1

#define MV_DETA_TYPE char
#define NP_DETA_TYPE short

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



void set_motionvector(const std::string motion_vector_file, std::vector<mv_class>& mv_all, int cframe) {
	// ファイル内のデータが膨大すぎるため、埋め込み時に適宜この関数を呼び出して、その都度ファイルからデータを読み出す
	// 埋め込む先頭フレーム(cframe)までファイル読み込みを飛ばしてからnum_embedframe分データを取得する

	std::ifstream ifs;
	ifs.open(motion_vector_file);
	if (ifs.fail()) {
		std::cerr << "can't open motion vector data file\n";
		std::getchar();
		exit(3);
	}

	std::string str;
	std::string cframe_str = std::to_string(cframe);
	cframe_str.insert(0, "pts=");
	while (str.find(cframe_str) == std::string::npos) {   //pts=cframeとなるまで飛ばす
		getline(ifs, str, ' ');
	}


	mv_class temp_class;
	// 初期化
	temp_class.frame_index = -1;
	temp_class.x_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);
	temp_class.y_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);

	std::vector<int> debug_array(120);// debug用

	for (int pts = 1; pts < num_embedframe + 1 && !ifs.eof();) {
		int temp_start;
		std::string temp_str;
		int temp_count = 0;


		getline(ifs, str, ' ');


		if (str.find("pts") != std::string::npos) {          // ptsが出れば、1フレームとみなす 
			pts++;
		}
		else if (str.find("frame_index") != std::string::npos) {
			temp_start = str.find("frame_index");
			temp_str = str.substr(temp_start + 12, str.length());        //動きベクトルファイル内では、frame_index=数字になっているという前提
			if (atoi(temp_str.c_str()) == -1) {
				temp_class.frame_index = atoi(temp_str.c_str());
			}
			else {
				temp_class.frame_index = pts - 1 + cframe;
			}
		}
		else if (str.find("shape") != std::string::npos) {
			temp_start = str.find("shape");

			for (int i = 0; i < 120 * matrix_height; i++) {
				getline(ifs, str, '\t');

				if (i == 0) {
					temp_str = str.substr(13, str.length() - 13);             //この行大丈夫かな・・・・
				}
				else if (i % 120 == 0) {   // 行の切れ目の\nを削除
					temp_str = str.substr(1, str.length() - 1);              // この行ダイジョブかな・・
				}
				else {
					temp_str = str;
				}

				if (i < 120 * matrix_height / 2) {
					temp_class.x_vector.at<MV_DETA_TYPE>(i / 120, i % 120) = atoi(temp_str.c_str());
				}
				else {
					temp_class.y_vector.at<MV_DETA_TYPE>((i / 2) / 120, (i / 2) % 120) = atoi(temp_str.c_str());
				}
			}


			// 深いコピー
			mv_all[pts - 1].frame_index = temp_class.frame_index;
			mv_all[pts - 1].x_vector = temp_class.x_vector.clone();
			mv_all[pts - 1].y_vector = temp_class.y_vector.clone();


			// 再度初期化
			temp_class.frame_index = -1;
			temp_class.x_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);
			temp_class.y_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);

		}
	}
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


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int delta,  const std::vector<char>& embed,  int cframe, std::string motion_vector_file) {  // luminanceにはmフレームの輝度値が含まれている
	std::vector<cv::Mat> means;
	std::vector<cv::Mat> deviations;
	cv::Mat m_means = cv::Mat::zeros(FRAME_height -8 , FRAME_width, CV_32F);  // mフレームでの"ブロックごとの輝度平均値"の平均値
	std::vector<cv::Mat> m_deviations;
	int i, j, x, y;
	std::vector<mv_class> mv_all(num_embedframe);        // 元ファイルから読み込んだ動きベクトルのデータを整理してすべて格納

	// ファイルからデータ取得
	set_motionvector(motion_vector_file, mv_all, cframe);


	for (i = 0; i < num_embedframe; i++) {
		means.push_back(filter(luminance[i]));          // ブロックごとに均した輝度値配列をmeansに格納する．
		luminance[i].convertTo(luminance[i], CV_32F);
		deviations.push_back(luminance[i] - means[i]);  // 各画素輝度値-"ブロックごとの平均輝度値"の結果を偏差(deviations)に格納する
	}

	m_means = means[0].clone() / num_embedframe;
	for (i = 1; i < num_embedframe; i++) {
		m_means += means[i] / num_embedframe;
	}

	for (i = 0; i < num_embedframe; i++) {
		m_deviations.push_back(means[i] - m_means);    // ブロックごとの平均輝度値 - mフレーム間での"ブロックごとの平均輝度値"の平均値
	}


	//// debug 
	int debug_xx;
	std::cout << "本来の輝度値" << std::endl;
	for (int j = 0; j <= 19; j++) {
		for (int k = 0; k <= 3; k++) {
			debug_xx = means[j].at<float>(0, k*block_size);
			std::cout << debug_xx << " ";
		}
		std::cout << std::endl;
	}
	//end debug

	// 変数用意
	std::vector<cv::Mat> Is_stop;
	std::vector<cv::Mat> Is_move;
	std::vector<cv::Mat> result_lumi;
	std::vector<cv::Mat> lumi_map;
	std::vector<cv::Mat> comp;

	for (int i = 0; i < num_embedframe; i++) {
		Is_stop.push_back(cv::Mat::zeros(cv::Size(FRAME_width / block_width, FRAME_height / block_height), CV_16SC1));
		Is_move.push_back(cv::Mat::zeros(cv::Size(FRAME_width / block_width, FRAME_height / block_height), CV_8UC1));
		result_lumi.push_back(cv::Mat::zeros(cv::Size(FRAME_width / block_width, FRAME_height / block_height), CV_32F));
		lumi_map.push_back(cv::Mat::zeros(cv::Size(FRAME_width / block_width, FRAME_height / block_height), CV_32F));
		comp.push_back(cv::Mat::zeros(cv::Size(FRAME_width / block_width, FRAME_height / block_height), CV_8UC1));

		for (int y = 0; y < FRAME_height / block_size; y++) {
			for (int x = 0; x < FRAME_width / block_size; x++) {
				lumi_map[i].at<float>(y, x) = -1;
			}
		}
	}

	// 変数用意終了


	for (int i = num_embedframe - 1; i > 0; i--) {       // 先頭フレームの動きベクトルは必要ない
		for (int y = 0; y < FRAME_height / block_size; y++) {
			for (int x = 0; x < FRAME_width / block_size; x++) {
				if (Is_there_mv(mv_all, cframe + i)) {  // これなくても十分	
					std::pair<int, int> back_point = get_back_pos(mv_all, cframe + i, y, x);    //フレーム番号とptsがごっちゃになっていないか確認する(現在のフレームを返せばいいと思われ)

					if (back_point.first != y || back_point.second != x) {                  // 他の画素位置に移動しているなら
						if (Is_stop[i].at<short>(y, x) == -1) {     // 既に移動していた場合
							Is_stop[i].at<short>(y, x) = 2;
						}
						else {
							Is_stop[i].at<short>(y, x) = 1;             // 移動前の位置にチェック
						}

						Is_stop[i - 1].at<short>(back_point.first, back_point.second) = -1;    // 移動先にもチェック
						Is_move[i].at<unsigned char>(y, x) = 1;
					}
				}
				else {
					// 継続条件を満たさないようにしてforを2つ抜ける
					y = FRAME_height / block_size - 1;
					x = FRAME_width / block_size - 1;
				}
			}
		}
	}


		// debug
	std::cout << "aa " << std::endl;
	int debug_dd = 0;
	for (int i = 0; i <= 19; i++) {
		for (int j = 0; j <= 2; j++) {
			debug_dd = Is_stop[i].at<short>(0, j);
			std::cout << debug_dd;
		}
		std::cout << std::endl;
	}


	//↑ここまではできている
	// debug end



	// 20フレームの最後から順に動きベクトルを調べて20個の輝度を確保する
	int num;          // 現在の画素に割り当てるべき透かしビットを格納
	// lumi_map求める

	for (int y = 0; y < FRAME_height / block_size; y++) {
		for (int x = 0; x < FRAME_width / block_size; x++) {
			lumi_map[num_embedframe - 1].at<float>(y, x) = means[num_embedframe - 1].at<float>(y *  block_size, x *  block_size);
			comp[num_embedframe - 1].at<unsigned char>(y, x) = 1;

			int temp_y = y;
			int temp_x = x;
			int jump_flg = 0;
			for (int i = num_embedframe - 1; i > 0; i--) {
				if (Is_stop[i].at<short>(temp_y, temp_x) == 1 || (Is_stop[i].at<short>(temp_y, temp_x) == 2 && jump_flg == 1)) {  // 他の画素に移動しているなら
					std::pair<int, int> back_point;
					back_point = get_back_pos(mv_all, cframe + i, temp_y, temp_x);

					lumi_map[i - 1].at<float>(y, x) = means[i - 1].at<float>(back_point.first *  block_size, back_point.second *  block_size);
					comp[i - 1].at<unsigned char>(back_point.first, back_point.second) = 1;
					temp_y = back_point.first;
					temp_x = back_point.second;
					jump_flg = 1;
				}
				else if ((Is_stop[i].at<short>(temp_y, temp_x) == -1 && jump_flg != 1) || (Is_stop[i].at<short>(temp_y, temp_x) == 2 && jump_flg != 1)) {       // 移動せず上書きされたならこれ以上lumi_mapには何も読み込まない
					break;
				}
				else {
					lumi_map[i - 1].at<float>(y, x) = means[i - 1].at<float>(temp_y *  block_size, temp_x *  block_size);
					comp[i - 1].at<unsigned char>(temp_y, temp_x) = 1;
					jump_flg = 0;
				}
			}
		}
	}



	//debug
	int debug = 0;

	std::cout << "lumi check" << std::endl;
	for (int l = 0; l < 19; l++) {
		for (int k = 0; k < 5; k++) {
			debug = lumi_map[l].at<float>(0, k);
			std::cout << debug << " ";
		}
		std::cout << std::endl;
	}

	// end debug

	std::vector<std::vector<float>> vec;
	vec.resize(BG_width * BG_height);         // vecのサイズ変更, vecはブロック群の中のブロックの個数分のサイズを持っている.

	int success_num = 0;


	for (y = 0; y < FRAME_height / block_height; y++) {
		for (x = 0; x < FRAME_width / block_width; x++) {
			int no_embed_flg = 0;

			float temp_ave = 0;
			std::vector<float> temp_devi(num_embedframe, 0);

			for (int i = 0; i < num_embedframe; i++) {
				if (lumi_map[i].at<float>(y, x) == -1) {          // 上書きされている場合
					no_embed_flg = 1;
				}
				temp_ave += lumi_map[i].at<float>(y, x);
			}
			temp_ave /= num_embedframe;


			if (no_embed_flg == 1) {     // 上書きされているなら復号しない
				continue;
			}
			else {
				for (int i = 0; i < num_embedframe; i++) {
					temp_devi[i] = lumi_map[i].at<float>(y, x) - temp_ave;
				}

				float sum = 0.00;
				for (i = 0; i < num_embedframe; i++) {
					sum += temp_devi[i] * temp_devi[i];
				}
				vec[x % BG_width + (y % BG_height) *BG_width].push_back(sqrt(sum / num_embedframe));

				success_num++;    //20フレーム間でブロックを20個確保できた場合
			}
		}
	}

	std::cout << "success" << success_num << std::endl;

	// 閾値を求める
	std::vector<size_t> n_count;
	float threshold = 0;
	delta_thisfile = delta;
	int vec_count = 0;

	for (i = 0; i < BG_width * BG_height; i++) { // フレームのすべてのブロックについて標準偏差がδより小さいものの個数をカウントする．
		n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));    // vecの中で，deltaより値が小さいものの個数をカウント
	
		threshold += n_count[i];    // その個数を閾値に加える．
	}

	threshold = threshold / success_num;  // それをブロック群個数で割る．平均化する     
													 // 閾値求め終わり

	for (i = 0; i < BG_width * BG_height; i++) {  // どれだけδ未満の画素値のものがあったか判断する．閾値以上であれば0で復号
		if (n_count[i] >= threshold * vec[i].size())
			decode->push_back('0');
		else
			decode->push_back('1');
	}
}




int ptob(int pixel_pos) {
	return pixel_pos * block_size / motionvector_block_size;     // この辺も　もう一度チェックする
}

int btop(int block_pos) {   // 絶対値が０でないが１６未満の場合は正の整数なら１，負の整数ならー１を返す
	if (block_pos < 0 && block_pos > -16) {
		return -1;
	}
	else if (block_pos > 0 && block_pos < 16) {
		return 1;
	}

	return block_pos / block_size;
}


// mv_all[未来の座標] = 未来の座標 - 過去の座標
std::pair<int, int > get_back_pos(std::vector<mv_class>& mv_all, int frame, int y, int x) {
	// フレーム番号と座標を与えるとその次の座標を返す)
	// 何も動いていなければ元の座標を返す
	int bl_y = ptob(y);
	int bl_x = ptob(x);

	std::pair<int, int> back_point;

	if (motionvector_block_size == 16) {       // フレームの縦幅が1080の時はマクロブロックサイズ16では割り切れず，8画素分余る．その場合，そのまま座標を返す
		if (y == 134) {
			back_point.first = y;
			back_point.second = x;

			return back_point;
		}
	}

	//// debug
	//int de_bly = ptob(0);
	//int de_blx = ptob(2);
	//int debug_tempy = mv_all[3].y_vector.at<MV_DETA_TYPE>(de_bly, de_blx);
	//int debug_tempx = mv_all[3].x_vector.at<MV_DETA_TYPE>(de_bly, de_blx);

	//back_point = std::make_pair(0 - btop(debug_tempy), 2 - btop(debug_tempx));

	//std::cout << back_point.second << std::endl;

	//for (int i = 0; i < 5; i++) {
	//	mv_all[9].x_vector;
	//}


	////end debug

	int temp_y = mv_all[frame % num_embedframe].y_vector.at<MV_DETA_TYPE>(bl_y, bl_x);
	int temp_x = mv_all[frame % num_embedframe].x_vector.at<MV_DETA_TYPE>(bl_y, bl_x);


	back_point = std::make_pair(y - btop(temp_y), x - btop(temp_x));


	// 現状のエラーをとりあえず改善するため
	if (back_point.first < 0 || back_point.first >= (FRAME_height / block_size)) {       // ここが少しおかしいか
		back_point.first = y;
	}
	if (back_point.second < 0 || back_point.second >= (FRAME_width / block_size)) {     // ここが少しおかしいか
		back_point.second = x;
	}

	return back_point;
}


bool Is_there_mv(std::vector<mv_class> &mv_all, int frame) {  // ダミー場合はfalseを返す

	if (mv_all[frame % num_embedframe].frame_index == -1) {
		return false;
	}
	else {
		return true;
	}
}