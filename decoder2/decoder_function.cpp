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



void set_motionvector(const std::string motion_vector_file, std::vector<mv_class>& mv_all, int cframe) {
	// �t�@�C�����̃f�[�^���c�傷���邽�߁A���ߍ��ݎ��ɓK�X���̊֐����Ăяo���āA���̓s�x�t�@�C������f�[�^��ǂݏo��
	// ���ߍ��ސ擪�t���[��(cframe)�܂Ńt�@�C���ǂݍ��݂��΂��Ă���num_embedframe���f�[�^���擾����

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
	while (str.find(cframe_str) == std::string::npos) {   //pts=cframe�ƂȂ�܂Ŕ�΂�
		getline(ifs, str, ' ');
	}


	mv_class temp_class;
	// ������
	temp_class.frame_index = -1;
	temp_class.x_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);
	temp_class.y_vector = cv::Mat::zeros(cv::Size(FRAME_width / motionvector_block_size, FRAME_height / motionvector_block_size), MV_DETA_SIZE);

	std::vector<int> debug_array(120);// debug�p

	for (int pts = 1; pts < num_embedframe + 1 && !ifs.eof();) {
		int temp_start;
		std::string temp_str;
		int temp_count = 0;


		getline(ifs, str, ' ');


		if (str.find("pts") != std::string::npos) {          // pts���o��΁A1�t���[���Ƃ݂Ȃ� 
			pts++;
		}
		else if (str.find("frame_index") != std::string::npos) {
			temp_start = str.find("frame_index");
			temp_str = str.substr(temp_start + 12, str.length());        //�����x�N�g���t�@�C�����ł́Aframe_index=�����ɂȂ��Ă���Ƃ����O��
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
					temp_str = str.substr(13, str.length() - 13);             //���̍s���v���ȁE�E�E�E
				}
				else if (i % 120 == 0) {   // �s�̐؂�ڂ�\n���폜
					temp_str = str.substr(1, str.length() - 1);              // ���̍s�_�C�W���u���ȁE�E
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


			// �[���R�s�[
			mv_all[pts - 1].frame_index = temp_class.frame_index;
			mv_all[pts - 1].x_vector = temp_class.x_vector.clone();
			mv_all[pts - 1].y_vector = temp_class.y_vector.clone();


			// �ēx������
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


void decoding(std::vector<cv::Mat> luminance, std::vector<char>* decode, int delta,  const std::vector<char>& embed,  int cframe, std::string motion_vector_file) {  // luminance�ɂ�m�t���[���̋P�x�l���܂܂�Ă���
	std::vector<cv::Mat> means;
	std::vector<cv::Mat> deviations;
	cv::Mat m_means = cv::Mat::zeros(FRAME_height -8 , FRAME_width, CV_32F);  // m�t���[���ł�"�u���b�N���Ƃ̋P�x���ϒl"�̕��ϒl
	std::vector<cv::Mat> m_deviations;
	int i, j, x, y;
	std::vector<mv_class> mv_all(num_embedframe);        // ���t�@�C������ǂݍ��񂾓����x�N�g���̃f�[�^�𐮗����Ă��ׂĊi�[

	// �t�@�C������f�[�^�擾
	set_motionvector(motion_vector_file, mv_all, cframe);


	for (i = 0; i < num_embedframe; i++) {
		means.push_back(filter(luminance[i]));          // �u���b�N���Ƃɋς����P�x�l�z���means�Ɋi�[����D
		luminance[i].convertTo(luminance[i], CV_32F);
		deviations.push_back(luminance[i] - means[i]);  // �e��f�P�x�l-"�u���b�N���Ƃ̕��ϋP�x�l"�̌��ʂ�΍�(deviations)�Ɋi�[����
	}

	m_means = means[0].clone() / num_embedframe;
	for (i = 1; i < num_embedframe; i++) {
		m_means += means[i] / num_embedframe;
	}

	for (i = 0; i < num_embedframe; i++) {
		m_deviations.push_back(means[i] - m_means);    // �u���b�N���Ƃ̕��ϋP�x�l - m�t���[���Ԃł�"�u���b�N���Ƃ̕��ϋP�x�l"�̕��ϒl
	}


	//// debug 
	int debug_xx;
	std::cout << "�{���̋P�x�l" << std::endl;
	for (int j = 0; j <= 19; j++) {
		for (int k = 0; k <= 3; k++) {
			debug_xx = means[j].at<float>(0, k*block_size);
			std::cout << debug_xx << " ";
		}
		std::cout << std::endl;
	}
	//end debug

	// �ϐ��p��
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

	// �ϐ��p�ӏI��


	for (int i = num_embedframe - 1; i > 0; i--) {       // �擪�t���[���̓����x�N�g���͕K�v�Ȃ�
		for (int y = 0; y < FRAME_height / block_size; y++) {
			for (int x = 0; x < FRAME_width / block_size; x++) {
				if (Is_there_mv(mv_all, cframe + i)) {  // ����Ȃ��Ă��\��	
					std::pair<int, int> back_point = get_back_pos(mv_all, cframe + i, y, x);    //�t���[���ԍ���pts����������ɂȂ��Ă��Ȃ����m�F����(���݂̃t���[����Ԃ��΂����Ǝv���)

					if (back_point.first != y || back_point.second != x) {                  // ���̉�f�ʒu�Ɉړ����Ă���Ȃ�
						if (Is_stop[i].at<short>(y, x) == -1) {     // ���Ɉړ����Ă����ꍇ
							Is_stop[i].at<short>(y, x) = 2;
						}
						else {
							Is_stop[i].at<short>(y, x) = 1;             // �ړ��O�̈ʒu�Ƀ`�F�b�N
						}

						Is_stop[i - 1].at<short>(back_point.first, back_point.second) = -1;    // �ړ���ɂ��`�F�b�N
						Is_move[i].at<unsigned char>(y, x) = 1;
					}
				}
				else {
					// �p�������𖞂����Ȃ��悤�ɂ���for��2������
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


	//�������܂ł͂ł��Ă���
	// debug end



	// 20�t���[���̍Ōォ�珇�ɓ����x�N�g���𒲂ׂ�20�̋P�x���m�ۂ���
	int num;          // ���݂̉�f�Ɋ��蓖�Ă�ׂ��������r�b�g���i�[
	// lumi_map���߂�

	for (int y = 0; y < FRAME_height / block_size; y++) {
		for (int x = 0; x < FRAME_width / block_size; x++) {
			lumi_map[num_embedframe - 1].at<float>(y, x) = means[num_embedframe - 1].at<float>(y *  block_size, x *  block_size);
			comp[num_embedframe - 1].at<unsigned char>(y, x) = 1;

			int temp_y = y;
			int temp_x = x;
			int jump_flg = 0;
			for (int i = num_embedframe - 1; i > 0; i--) {
				if (Is_stop[i].at<short>(temp_y, temp_x) == 1 || (Is_stop[i].at<short>(temp_y, temp_x) == 2 && jump_flg == 1)) {  // ���̉�f�Ɉړ����Ă���Ȃ�
					std::pair<int, int> back_point;
					back_point = get_back_pos(mv_all, cframe + i, temp_y, temp_x);

					lumi_map[i - 1].at<float>(y, x) = means[i - 1].at<float>(back_point.first *  block_size, back_point.second *  block_size);
					comp[i - 1].at<unsigned char>(back_point.first, back_point.second) = 1;
					temp_y = back_point.first;
					temp_x = back_point.second;
					jump_flg = 1;
				}
				else if ((Is_stop[i].at<short>(temp_y, temp_x) == -1 && jump_flg != 1) || (Is_stop[i].at<short>(temp_y, temp_x) == 2 && jump_flg != 1)) {       // �ړ������㏑�����ꂽ�Ȃ炱��ȏ�lumi_map�ɂ͉����ǂݍ��܂Ȃ�
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
	vec.resize(BG_width * BG_height);         // vec�̃T�C�Y�ύX, vec�̓u���b�N�Q�̒��̃u���b�N�̌����̃T�C�Y�������Ă���.

	int success_num = 0;


	for (y = 0; y < FRAME_height / block_height; y++) {
		for (x = 0; x < FRAME_width / block_width; x++) {
			int no_embed_flg = 0;

			float temp_ave = 0;
			std::vector<float> temp_devi(num_embedframe, 0);

			for (int i = 0; i < num_embedframe; i++) {
				if (lumi_map[i].at<float>(y, x) == -1) {          // �㏑������Ă���ꍇ
					no_embed_flg = 1;
				}
				temp_ave += lumi_map[i].at<float>(y, x);
			}
			temp_ave /= num_embedframe;


			if (no_embed_flg == 1) {     // �㏑������Ă���Ȃ畜�����Ȃ�
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

				success_num++;    //20�t���[���ԂŃu���b�N��20�m�ۂł����ꍇ
			}
		}
	}

	std::cout << "success" << success_num << std::endl;

	// 臒l�����߂�
	std::vector<size_t> n_count;
	float threshold = 0;
	delta_thisfile = delta;
	int vec_count = 0;

	for (i = 0; i < BG_width * BG_height; i++) { // �t���[���̂��ׂẴu���b�N�ɂ��ĕW���΍�����菬�������̂̌����J�E���g����D
		n_count.push_back(std::count_if(vec[i].begin(), vec[i].end(), is_less_than));    // vec�̒��ŁCdelta���l�����������̂̌����J�E���g
	
		threshold += n_count[i];    // ���̌���臒l�ɉ�����D
	}

	threshold = threshold / success_num;  // ������u���b�N�Q���Ŋ���D���ω�����     
													 // 臒l���ߏI���

	for (i = 0; i < BG_width * BG_height; i++) {  // �ǂꂾ�������̉�f�l�̂��̂������������f����D臒l�ȏ�ł����0�ŕ���
		if (n_count[i] >= threshold * vec[i].size())
			decode->push_back('0');
		else
			decode->push_back('1');
	}
}




int ptob(int pixel_pos) {
	return pixel_pos * block_size / motionvector_block_size;     // ���̕ӂ��@������x�`�F�b�N����
}

int btop(int block_pos) {   // ��Βl���O�łȂ����P�U�����̏ꍇ�͐��̐����Ȃ�P�C���̐����Ȃ�[�P��Ԃ�
	if (block_pos < 0 && block_pos > -16) {
		return -1;
	}
	else if (block_pos > 0 && block_pos < 16) {
		return 1;
	}

	return block_pos / block_size;
}


// mv_all[�����̍��W] = �����̍��W - �ߋ��̍��W
std::pair<int, int > get_back_pos(std::vector<mv_class>& mv_all, int frame, int y, int x) {
	// �t���[���ԍ��ƍ��W��^����Ƃ��̎��̍��W��Ԃ�)
	// ���������Ă��Ȃ���Ό��̍��W��Ԃ�
	int bl_y = ptob(y);
	int bl_x = ptob(x);

	std::pair<int, int> back_point;

	if (motionvector_block_size == 16) {       // �t���[���̏c����1080�̎��̓}�N���u���b�N�T�C�Y16�ł͊���؂ꂸ�C8��f���]��D���̏ꍇ�C���̂܂܍��W��Ԃ�
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


	// ����̃G���[���Ƃ肠�������P���邽��
	if (back_point.first < 0 || back_point.first >= (FRAME_height / block_size)) {       // ��������������������
		back_point.first = y;
	}
	if (back_point.second < 0 || back_point.second >= (FRAME_width / block_size)) {     // ��������������������
		back_point.second = x;
	}

	return back_point;
}


bool Is_there_mv(std::vector<mv_class> &mv_all, int frame) {  // �_�~�[�ꍇ��false��Ԃ�

	if (mv_all[frame % num_embedframe].frame_index == -1) {
		return false;
	}
	else {
		return true;
	}
}