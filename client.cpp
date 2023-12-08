extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
}

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>


const int Gbit = 1024*1024*128;		// 1Gb in byte
const int Bms = 1024*128;			// to enable 1Gbps, min data size in byte needed per ms


void ms_sleep(double ms)
{
	return;
}


/* read rate from csv file */
std::vector<double> read_csv_rate(const std::string& filename)
{
    std::vector<double> rate_data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open CSV file");
    }

	/* read each line */
    std::string line;
    while (std::getline(file, line)) {
		std::string val;
		val.push_back(line[0]);
		rate_data.push_back(std::stod(val));
    }
    file.close();
    return rate_data;
}

/* rate control data transfer */
int send_by_rate(char* data, std::vector<double> rates, int sockfd)
{
	for (auto rate = rates.cbegin(); rate != rates.cend(); rate++) {
		/* send data in rate control */
		int offset = 0;
		double over_head = 0; // negative value
		if (data) {
			std::chrono::high_resolution_clock::time_point s0 = std::chrono::high_resolution_clock::now();
			for (int j = 0; j < 256; j++) {
				/* control the data transfer per ms */
				int ret = 0;
				std::chrono::high_resolution_clock::time_point s = std::chrono::high_resolution_clock::now();
				while(ret == 0)
					ret = send(sockfd, data + offset, (size_t)(Bms *(*rate)*4), 0);
				std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::now();

				/* rate control */
				if (ret == -1) {
					perror("[-]Error in sending file.");
					return EXIT_FAILURE;
				} else {
					/* check if sleep */
					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();
					double durationms = (double) ((double)duration/(double)1000);
					double sleep_ms = (double)((double)(ret*8))/((double)(*rate)*1024*1024*1024)*1000 - durationms;
					sleep_ms += over_head;
					// std::cout << j << " " << durationms << "ms " << "sleep " << sleep_ms << "ms " << ret << "B " << "total " << offset+ret << "B " << std::endl;
					
					/* sleep */
					if (sleep_ms > 0) {
						/* sleep for sleep_ms, use 800 to reduce error */
						std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(sleep_ms*800)));
					} else {
						over_head += sleep_ms;
					}
					offset += ret;
					continue;
				}
			}
			std::chrono::high_resolution_clock::time_point e0 = std::chrono::high_resolution_clock::now();
			auto total = std::chrono::duration_cast<std::chrono::microseconds>(e0 - s0).count();
			double totalms = (double) ((double)total/(double)1000);
			std::cout << "Total_Time=" << totalms << "ms Total_Size=" << offset/1024/1024 << "MB " << "Actual_Rate=" << ((double)offset*8/1024/1024/1024)/(totalms/1000) 
				<< "Gbs Expect_Rate=" << *rate << "Gbs" << std::endl;
		} else
			return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}


int main(int argc, char* argv[])
{
	if (argc < 4) {
		std::cout << "Usage: ./client <ip_addr> <data_file_path> <rates_csv>" << std::endl;
		return EXIT_FAILURE;
	}

	/* set up connection */
	std::string ip = argv[1];
	int port = 8080;
	struct sockaddr_in server_addr;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		perror("[-]Error in socket");
		exit(1);
	} printf("[+]Server socket created successfully.\n");

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(e == -1) {
		perror("[-]Error in socket");
		exit(1);
	} printf("[+]Connected to Server.\n");


	/* prepare for the data transfer */
	/* open data files */
	std::string filename = argv[2];
	std::ifstream data_stream(filename, std::ios::in);
	if (!data_stream.is_open()) {
		perror("[-]Error in open data file");
		return EXIT_FAILURE;
	} std::cout << "[+]File open success." << std::endl;

	/* read data from file */
	char* data = (char*) aligned_alloc(4096, (size_t)Gbit*4);
	data_stream.read(data, Gbit*4);
	if (data_stream.bad()) {
		perror("[-]Error in data read");
		return EXIT_FAILURE;
	} std::cout << "[+]Data read success." << std::endl;

	/* open csv rate file */
	std::string csvname = argv[3];
	std::vector<double> rates = read_csv_rate(csvname);
	
	/* setup thread and join */
	std::thread send_thread(send_by_rate, data, rates, sockfd);
	send_thread.join();

	/* close */
	data_stream.close();
	close(sockfd);

	return 0;
}
