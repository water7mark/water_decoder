#include "decoder.h"

void log_write(std::string read_file, std::string write_file)
{
	char date[64];
	std::ofstream ofs(log_file, std::ios::app);   // �㏑��
	time_t t = time(NULL);
	tm now;
	errno_t error;

	if (!ofs)
	{
		std::cout << "�t�@�C�����J���܂���ł����B" << std::endl;
		std::cin.get();
		exit(9);
	}

	error = localtime_s(&now, &t);

	strftime(date, sizeof(date), "%Y/%m/%d %a %H:%M:%S\n", &now);
	ofs << date << "decoder::" << "read_file�E" << read_file << "\n" << "write_file�E" << write_file << "\n" << std::endl;
	ofs << "----------------------------------------------\n" << std::endl;
	std::cout << "read_file:" << read_file << std::endl;
	std::cout << "write_file:" << write_file << std::endl;
}

void str_checker(std::string read_file, std::string write_file) {
	// �{���́C�A�z�z����g���ׂ��ł�?   // ���������͊֐������ׂ�
	std::vector<std::string> r_label = { "basket","library","lego", "walk1", "walk2" };
	std::vector<std::string> w_label = { "basket","library","lego", "walk1", "walk2" };
	std::vector<std::string> m_array = { "m10", "m20", "m30" };
	std::vector<std::string> delta_array = { "d1" "d2", "d3" ,"d5", "d10" ,"d1_5" };


	if (write_file.find("test") != std::string::npos) {
		return;
	}

	// read_file , write_file check
	for (int i = 0; i < end(r_label) - begin(r_label); i++) {
		if (read_file.find(r_label[i]) != std::string::npos) {     // ���Y�����񂪊܂܂�Ă���
			if (write_file.find(w_label[i]) != std::string::npos) { // write_file�ɂ����������񂪊܂܂�Ă���
				break;
			}
			else {   // ���������񂪊܂܂�Ă��Ȃ��D�~�X���Ă�ꍇ
				std::cout << "error: read label is not equal write label" << std::endl;
				std::cout << "read_file:" << read_file << std::endl;
				std::cout << "write_file:" << write_file << std::endl;
				getchar();
				exit(0);
			}
		}
		else if (i == (end(r_label) - begin(r_label) - 1)) {
			std::cout << "error:1 wrong read_file name!!!!" << std::endl;
			getchar();
			exit(1);
		}
	}
}

bool overwrite_check(std::string write_file) {
	std::ifstream ifs;

	ifs.open(write_file);

	if (ifs) {
		std::cout << "error: overwrite:: write file is " << write_file << std::endl;
		getchar();
		return false;
	}
	else {
		return true;
	}
}


void change_filename(std::string& read_file, std::string& write_file, int loop_count, std::string& write_file_ex) {
	const std::string mp4_read_array[5] = { "basket","library", "lego", "walk1", "walk2" };
	const std::string read_array[5] = { "basket" , "library" , "lego", "walk1", "walk2" };
	const std::string write_array[5] = { "basket" , "library" , "lego", "walk1", "walk2" };
	int change_point = 0;
	int check_num = 0;

	// read_file�̖��O�𓮉�ɉ����ĕύX
	change_point = (int)read_file.find("xxx");
	read_file.replace(change_point, 3, mp4_read_array[loop_count - 1]);

	check_num = (int)read_file.rfind("ver");
	if (read_file[check_num + 5] - '0' != delta) {
		std::cout << "error: miss type:: read_file is wrong" << read_file << std::endl;
		getchar();
		exit(0);
	}

	// write_file�̖��O�𓮉�ɉ����ĕύX
	change_point = (int)write_file.find("xxx");
	write_file.replace(change_point, 3, read_array[loop_count - 1]);

	check_num = (int)write_file.rfind("ver");
	if (write_file[check_num + 5] - '0' != delta) {
		std::cout << "error: miss type:: write_file is wrong" << write_file << std::endl;
		getchar();
		exit(0);
	}

	change_point = (int)write_file_ex.find("xxx");
	write_file_ex.replace(change_point, 3, write_array[loop_count - 1]);


}