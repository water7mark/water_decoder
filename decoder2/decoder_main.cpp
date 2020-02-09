#include "decoder.h"

int main(int argc, char *argv[])
{
	for (int now_loop = 1; now_loop <= PROJECT_LOOP; now_loop++) {
		// initilize
		std::string read_file = basis_read_file;
		std::string write_file = basis_write_file;
		std::string motion_vector_file = basis_motion_vector_file;

		change_filename(read_file, write_file, now_loop, motion_vector_file);

		log_write(read_file, write_file);
		str_checker(read_file, write_file);


		cv::VideoCapture cap;
		std::vector<char> embed;
		std::vector<char> decode;  // 得られた復号ビットを一時的に格納
		std::ofstream ofs;
		//std::ofstream ofs_txt;
		cv::TickMeter meter;
		int framenum;
		cv::Size size;


		//initializing
		init_decoder(&cap, &embed, &size, &ofs, read_file, write_file, num_embedframe, delta);
		int number_of_frames = cap.get(CV_CAP_PROP_FRAME_COUNT) - 1;
		int i;
		std::vector<cv::Mat> planes;
		std::vector<cv::Mat> planes2;
		std::vector<cv::Mat> luminance;
		std::vector<cv::Mat> Cr;
		std::vector<cv::Mat> Cb;
		cv::Mat frame_BGR(size, CV_8UC3);
		cv::Mat frame_YCrCb(size, CV_8UC3);
		std::vector<cv::Mat> temp_lumi;
		std::vector<cv::Mat> temp_Cr;
		std::vector<cv::Mat> temp_Cb;


		//// 時間短縮
		//for (int i = 0; i < 297; i++) {
		//	cap >> frame_BGR;
		//}

		for (i = 0; i < (num_embedframe - 1); i++) {
			for (int j = 0; j < 3; j++) {
				planes.push_back(cv::Mat::zeros(cv::Size(FRAME_width, FRAME_height - 8), CV_8UC1));
				planes2.push_back(cv::Mat::zeros(cv::Size(FRAME_width, FRAME_height), CV_8UC1));
			}


			cap >> frame_BGR;
			if (frame_BGR.empty()) {
				break;
			}

			if (frame_BGR.size() != cv::Size(FRAME_width, FRAME_height)) {
				cv::resize(frame_BGR, frame_BGR, cv::Size(FRAME_width, FRAME_height));
			}

			cv::cvtColor(frame_BGR, frame_YCrCb, CV_BGR2YCrCb);

			//YCrCbチャンネルごとに分解
			cv::split(frame_YCrCb, planes2);


			for (int j = 0; j < 3; j++) {
				for (int y = 0; y < FRAME_height; y++) {
					for (int x = 0; x < FRAME_width; x++) {
						if (y < 16 * 67) {   // あまりの画素以外を格納
							planes[j].at<unsigned char>(y, x) = planes2[j].at<unsigned char>(y, x);
						}
					}
				}
			}


			luminance.push_back(planes[0]);
			Cr.push_back(planes[1]);
			Cb.push_back(planes[2]);
			planes.clear();
			planes2.clear();
		}

		
		
		//decoding
		do {
			for (int j = 0; j < 3; j++) {
				planes.push_back(cv::Mat::zeros(cv::Size(FRAME_width, FRAME_height - 8), CV_8UC1));
				planes2.push_back(cv::Mat::zeros(cv::Size(FRAME_width, FRAME_height), CV_8UC1));
			}

			//preprocessing
			cap >> frame_BGR;
			framenum = cap.get(CV_CAP_PROP_POS_FRAMES) - num_embedframe;

			if (frame_BGR.empty()) break;

			if (frame_BGR.size() != cv::Size(FRAME_width, FRAME_height)) {
				cv::resize(frame_BGR, frame_BGR, cv::Size(FRAME_width, FRAME_height));
			}
			cv::cvtColor(frame_BGR, frame_YCrCb, CV_BGR2YCrCb);

			//YCrCbチャンネルごとに分解
			cv::split(frame_YCrCb, planes2);


			for (int j = 0; j < 3; j++) {
				for (int y = 0; y < FRAME_height; y++) {
					for (int x = 0; x < FRAME_width; x++) {
						if (y < 16 * 67) {   // あまりの画素以外を格納
							planes[j].at<unsigned char>(y, x) = planes2[j].at<unsigned char>(y, x);
						}
					}
				}
			}

			luminance.push_back(planes[0]);   // luminanceには直前のnum_embedframeフレームの輝度値が含まれている
			Cr.push_back(planes[1]);
			Cb.push_back(planes[2]);
			planes.clear();


			decoding(luminance, &decode, delta, embed, framenum, motion_vector_file);

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

			std::cout << "frame " << cap.get(CV_CAP_PROP_POS_FRAMES) - (num_embedframe - 1) << " to " << cap.get(CV_CAP_PROP_POS_FRAMES) << " rate=" << rate << std::endl;

			ofs << "frame " << cap.get(CV_CAP_PROP_POS_FRAMES) - (num_embedframe - 1) << " to " << cap.get(CV_CAP_PROP_POS_FRAMES) << "," << correct << ","
				<< error << "," << cap.get(CV_CAP_PROP_POS_FRAMES) - (num_embedframe - 1) << "," << rate << ",,";
			//}

			for (std::vector<char>::iterator it = decode.begin(); it != decode.end(); ++it)
				ofs << *it << ",";
			ofs << std::endl;


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
