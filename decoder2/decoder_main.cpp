#include "decoder.h"

int main(int argc, char *argv[])
{
	for (int now_loop = 1; now_loop <= PROJECT_LOOP; now_loop++) {
		// initilize
		std::string read_file = basis_read_file;
		std::string write_file = basis_write_file;

		change_filename(read_file, write_file, now_loop);

		log_write(read_file, write_file);
		str_checker(read_file, write_file);

		//if (!overwrite_check(write_file)) {
		//	continue;
		//}

		cv::VideoCapture cap;
		std::vector<char> embed;
		std::vector<char> decode;  // 得られた復号ビットを一時的に格納
		std::ofstream ofs;
		//std::ofstream ofs_txt;
		cv::TickMeter meter;
		int framenum;
		cv::Size size;

		// 臨時で追加(n_countを出力)
		std::ofstream ofs2(write_file_ex);

		// 得られた復号ビットをすべて格納する(900*埋め込み透かしビットをm個に分ける)  ()
		std::vector<std::vector<std::vector<int>>> all_bit_array(m, std::vector<std::vector<int>>(1, std::vector<int>(DECODER_BIT)));

		//initializing
		init_decoder(&cap, &embed, &size, &ofs, read_file, write_file, m, delta);
		int number_of_frames = cap.get(CV_CAP_PROP_FRAME_COUNT) - 1;
		int i;
		std::vector<cv::Mat> planes(3);
		std::vector<cv::Mat> luminance;
		std::vector<cv::Mat> Cr;
		std::vector<cv::Mat> Cb;
		cv::Mat frame_BGR(size, CV_8UC3);
		cv::Mat frame_YCrCb(size, CV_8UC3);


		//// 時間短縮
		for (int i = 0; i < 297; i++) {
			cap >> frame_BGR;
		}

		for (i = 0; i < (m - 1); i++) {
			cap >> frame_BGR;
			if (frame_BGR.empty()) {
				break;
			}

			if (frame_BGR.size() != cv::Size(1920, 1080)) {
				cv::resize(frame_BGR, frame_BGR, cv::Size(1920, 1080));
			}

			cv::cvtColor(frame_BGR, frame_YCrCb, CV_BGR2YCrCb);

			//YCrCbチャンネルごとに分解
			cv::split(frame_YCrCb, planes);
			luminance.push_back(planes[0]);
			Cr.push_back(planes[1]);
			Cb.push_back(planes[2]);
			planes.clear();
		}


		//decoding
		do {
			//preprocessing
			cap >> frame_BGR;
			if (frame_BGR.empty()) break;

			if (frame_BGR.size() != cv::Size(1920, 1080)) {
				cv::resize(frame_BGR, frame_BGR, cv::Size(1920, 1080));
			}
			cv::cvtColor(frame_BGR, frame_YCrCb, CV_BGR2YCrCb);

			//YCrCbチャンネルごとに分解
			cv::split(frame_YCrCb, planes);
			luminance.push_back(planes[0]);   // luminanceには直前のmフレームの輝度値が含まれている
			Cr.push_back(planes[1]);
			Cb.push_back(planes[2]);
			planes.clear();



			ofs2 << "n_count" << "," << "sum" << std::endl;
			decoding(luminance, &decode, m, delta, ofs2, embed);

			int correct = 0, error = 0;
			double rate = 0.000;
			std::vector<char>::iterator it_e = embed.begin();
			std::vector<char>::iterator it_d = decode.begin();
			for (; it_d != decode.end() && it_e != embed.end(); ++it_e, ++it_d) {
				if (*it_e == *it_d)
					correct++;
				else
					error++;
			}
			rate = (double)correct / (correct + error) * 100;

			std::cout << "frame " << cap.get(CV_CAP_PROP_POS_FRAMES) - (m - 1) << " to " << cap.get(CV_CAP_PROP_POS_FRAMES) << " rate=" << rate << std::endl;

			ofs << "frame " << cap.get(CV_CAP_PROP_POS_FRAMES) - (m - 1) << " to " << cap.get(CV_CAP_PROP_POS_FRAMES) << "," << correct << ","
				<< error << "," << cap.get(CV_CAP_PROP_POS_FRAMES) - (m - 1) << "," << rate << ",,";
			//}

			for (std::vector<char>::iterator it = decode.begin(); it != decode.end(); ++it)
				ofs << *it << ",";
			ofs << std::endl;


			// 2018_11_22 add
			//if (cap.get(cap.get(CV_CAP_PROP_POS_FRAMES) > 300)) {
			//	assort(decode, all_bit_array, m);  // 復号ビット格納
			//}

			//std::cout << decode << std::endl;
			decode.clear();
			luminance.erase(luminance.begin());
			Cr.erase(Cr.begin());
			Cb.erase(Cb.begin());

			/*	meter.reset();
			meter.start();*/

		} while (cap.get(CV_CAP_PROP_POS_FRAMES) < 900);

		cap.release();

		//// 復号ビット決定
		//decide_bit()
	}
	return 0;
}
