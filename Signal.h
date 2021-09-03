#include <iostream>
#include <cstdio>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <vector>
#include <time.h> 
#include <condition_variable>  
#include <stdio.h> 
#include <fstream>
#include <algorithm>
#include <string.h>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <map>
#include <hiredis/hiredis.h>
#include <sys/select.h>
#include <sys/time.h>
#include<sys/types.h>
#include <math.h>
#include <unordered_map>
#include <numeric>
#include "xtp_trader_api.h"
#include "trade_spi.h"
#include "FileUtils.h"
#include "xtp_quote_api.h"
#include "quote_spi.h"
using namespace std;


typedef struct Trade_data
{
	string exchangeid;
	string contract;
	double price;
	int num;
}Trade_data;

typedef struct Trade_Signal
{
	string exchangeid;
	string contract;
	int hands;
	int trade_hands;
	double min_change;
	double ref;
}Trade_Signal;


class Signal
{

public:
	Signal();
	void make_startegy(char* *allInstruments_sh, char* *allInstruments_sz, int sh_num, int sz_num);
	void over();
private:
	static Signal* pThis;
	string main_time;
	string sub_time;
	redisContext* my_redis;
	double long_hold_pair_price =0;
	double short_hold_pair_price =0;
	int period = 5;
	double invest_money = 1000000;
	string today_symbols[2];
	string last_symbols[2];
	void start_buy(string exchangeid, string instrument, double price, int num, string order_type);
	void start_sell(string exchangeid, string instrument, double price, int num, string order_type);
	void start_trading(string contract, int volume, double min_change, string side, string exchange);
	void load_signal_data(string filename);
	void load_last_data(string filename);
	void check_hold_position();
	void place_order(string exchangeid, string instrument, double price, int num,  XTP_SIDE_TYPE side);
};
