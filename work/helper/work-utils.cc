#include "work-utils.h"

namespace ns3 {

string percentage(double number, double total) {
  return total == 0 ? "0.0%" : to_string(number / total * 100.0) + "%";
}

string formatTime(time_t mtime, const char *format) {
  char *buff = (char *)malloc(sizeof(char) * 20);
  struct tm *timeinfo;
  timeinfo = localtime(&mtime);
  strftime(buff, 20, format, timeinfo);
  return string(buff);
}

string formatTime(time_t mtime) {
  char *buff = (char *)malloc(sizeof(char) * 20);
  struct tm *timeinfo;
  timeinfo = localtime(&mtime);
  strftime(buff, 20, "%F %T", timeinfo);
  return string(buff);
}

void printFormattedTime(time_t mtime, ostream &out) {
  string str = formatTime(mtime);
  out << str;
}

void printFormattedTime(time_t mtime) {
  string str = formatTime(mtime);
  cout << str;
}

time_t strToTime(const char *str) {
  struct tm tm;
  strptime(str, "%F %T", &tm);
  // need to set daylight saving time to unkown for consistent parse
  tm.tm_isdst = -1;
  time_t t = mktime(&tm);
  return t;
}

int extractHour(time_t mtime) {
  struct tm *timeinfo;
  timeinfo = localtime(&mtime);
  return timeinfo->tm_hour;
}

int extractDay(time_t mtime) {
  struct tm *timeinfo;
  timeinfo = localtime(&mtime);
  return timeinfo->tm_mday;
}

string getTimeOfSimulationStart() {
  time_t now = time(0);
  tm *ltm = localtime(&now);
  ostringstream convert;

  convert << 1900 + ltm->tm_year << "-";

  if ((ltm->tm_mon + 1) < 10)
    convert << "0";
  convert << ltm->tm_mon + 1 << "-";

  if (ltm->tm_mday < 10)
    convert << "0";
  convert << ltm->tm_mday << "_";

  if (ltm->tm_hour < 10)
    convert << "0";
  convert << ltm->tm_hour << "-";

  if (ltm->tm_min < 10)
    convert << "0";
  convert << ltm->tm_min;

  return convert.str();
}

} // namespace ns3