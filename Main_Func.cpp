#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <string.h>
#include <hiredis/hiredis.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include "xtp_trader_api.h"
#include "trade_spi.h"
#include "FileUtils.h"
#include "xtp_quote_api.h"
#include "quote_spi.h"
#include "Signal.h"
using namespace std;

unordered_map<string, string> md_config;
XTP::API::TraderApi* pUserApi;
bool is_connected_ = false;
std::string trade_server_ip;
int trade_server_port;
uint64_t session_id_ = 0;
std::map<uint64_t,uint64_t> map_session;
uint64_t* session_arrary = NULL;
FileUtils* fileUtils = NULL;
XTPOrderInsertInfo* orderList = NULL;
std::string quote_server_ip;
int quote_server_port;
std::string quote_username;
std::string quote_password;
XTP_PROTOCOL_TYPE quote_protocol = XTP_PROTOCOL_UDP;

string erase_num(string data)
{
	string::iterator it = data.begin();
    while (it != data.end()) {
        if ((*it >= '0') && (*it <= '9')) {
            it = data.erase(it);
        } else { // 必须加else
            it++;
        }
    }
	return data;
}

void read_config(string file_name)
{
	int cfg_line = 0;
	//定义缓存字符串变量
	string st;
	string key, value;
	//定义文件变量
	ifstream infile;
	string::size_type idx;
	char* p = NULL, * q = NULL;
	//打开文件
	infile.open(file_name);

	//遍历直到文件的最后
	while (!infile.eof())
	{
		//初始化缓存
		st = "";
		//取得一行配置
		infile >> st;
		//找不到等号则继续
		idx = st.find(",");
		if (idx == string::npos)
		{
			continue;
		}
		//截断字符串得到key和value字符串
		key = st.substr(0, idx);
		value = st.substr(idx + 1, st.length() - idx);
		md_config[key] = value;
		//插入链表
	}
	//关闭文件
	infile.close();
}

void td_run()
{
	FileUtils* fileUtils = new FileUtils();
	if (!fileUtils->init())
	{
		std::cout << "The config.json file parse error." << std::endl;
#ifdef _WIN32
		system("pause");
#endif

		return;
	}
	trade_server_ip = fileUtils->stdStringForKey("trade_ip");
	trade_server_port = fileUtils->intForKey("trade_port");
	int out_count = fileUtils->intForKey("out_count");
	bool auto_save = fileUtils->boolForKey("auto_save");
	int client_id = fileUtils->intForKey("client_id");
	int account_count = fileUtils->countForKey("account");
	int resume_type = fileUtils->intForKey("resume_type");
	std::string account_key = fileUtils->stdStringForKey("account_key");
#ifdef _WIN32
	std::string filepath = fileUtils->stdStringForKey("path");
#else
	std::string filepath = fileUtils->stdStringForKey("path_linux");
#endif // _WIN32
	int32_t heat_beat_interval = fileUtils->intForKey("hb_interval");

	std::string account_name = fileUtils->stdStringForKey("account[%d].user", 0);
	std::string account_pw = fileUtils->stdStringForKey("account[%d].password", 0);

	pUserApi = XTP::API::TraderApi::CreateTraderApi(client_id,filepath.c_str(), XTP_LOG_LEVEL_DEBUG);	
	pUserApi->SubscribePublicTopic((XTP_TE_RESUME_TYPE)resume_type);
	cout<<"create trade api success"<<endl;
	pUserApi->SetSoftwareVersion("1.1.0"); 
	pUserApi->SetSoftwareKey(account_key.c_str());
	pUserApi->SetHeartBeatInterval(heat_beat_interval);
	MyTraderSpi* pUserSpi = new MyTraderSpi();
	pUserApi->RegisterSpi(pUserSpi);					
	pUserSpi->setUserAPI(pUserApi);
	pUserSpi->set_save_to_file(auto_save);
	if (out_count > 0)
	{
		pUserSpi->OutCount(out_count);
	}
	else
	{
		out_count = 1;
	}
	uint64_t temp_session_ = 0;
	std::cout << account_name << " login begin." << std::endl;
	temp_session_ = pUserApi->Login(trade_server_ip.c_str(), trade_server_port, account_name.c_str(), account_pw.c_str(), XTP_PROTOCOL_TCP); 
	cout<<"temp_session_="<<temp_session_<<endl;
	if (session_id_ == 0)
	{
		session_id_ = temp_session_;
	}
	if (temp_session_ > 0)
	{
		map_session.insert(std::make_pair(temp_session_, 0));
		cout<<"login to server success!!"<<endl;
	}
	else
	{
		XTPRI* error_info = pUserApi->GetApiLastError();
		std::cout << account_name << " login to server error, " << error_info->error_id << " : " << error_info->error_msg << std::endl;
	}
	session_arrary = new uint64_t[1];
	session_arrary[0] = temp_session_;
}

void start(char* *allInstruments_sh, char* *allInstruments_sz, int sh_num, int sz_num)
{
	cout<<"start strategy!"<<endl;
	Signal signal_obj = Signal();
	signal_obj.make_startegy(allInstruments_sh, allInstruments_sz, sh_num, sz_num);
}

int main()
{
	read_config("/root/other/xtp_trade/hs300.txt");
	redisContext* my_redis = redisConnect("127.0.0.1", 6379);
	int pair_num = 300;
	int sh_num = 0;
	int sz_num = 0;
	unordered_map<string, string>::iterator it;
	char* *allInstruments_sh = new char*[185];
	char* *allInstruments_sz = new char*[115];
	for (int i = 0; i < 185; i++) {
		allInstruments_sh[i] = new char[7];
		string instrument ="000000";
		strcpy(allInstruments_sh[i], instrument.c_str());
	}
	for (int i = 0; i < 115; i++) {
		allInstruments_sz[i] = new char[7];
		string instrument ="000000";
		strcpy(allInstruments_sz[i], instrument.c_str());
	}
	for(unordered_map<string, string>::iterator iter=md_config.begin();iter!=md_config.end();iter++)
	{
		string instrument = iter->first;
		string exchange = iter->second;
		// cout<<instrument<<", "<<exchange<<endl;
		if(exchange=="sh")
		{
			strcpy(allInstruments_sh[sh_num], instrument.c_str());
			sh_num++;
		}
		else if(exchange=="sz")
		{
			strcpy(allInstruments_sz[sz_num], instrument.c_str());
			sz_num++;
		}
	}
	cout<<"sh_num="<<sh_num<<endl;
	cout<<"sh_num="<<sz_num<<endl;

	thread t1(td_run);
	t1.detach();
	sleep(10);

	thread t2(start, allInstruments_sh, allInstruments_sz, sh_num, sz_num);
	t2.detach();

    while(1) {}
	return 0;
}
