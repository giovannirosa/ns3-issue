#ifndef UTILS
#define UTILS

#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

namespace ns3 {

/*
 * Format to percentage text
 * \param number dividend (numerator)
 * \param total divisor (denominator)
 * \returns formatted percentage string
 */
string percentage(double number, double total);

/*
 * Format from time_t to string using specific format
 * \param mtime time
 * \param format time format
 * \returns formatted time string
 */
string formatTime(time_t mtime, const char *format);

/*
 * Format from time_t to string using "%F %T" format
 * \param mtime time
 * \returns formatted time string
 */
string formatTime(time_t mtime);

/*
 * Print formatted time string to specific output stream
 * \param mtime time
 * \param out output stream
 */
void printFormattedTime(time_t mtime, ostream &out);

/*
 * Print formatted time string to standard output stream (cout)
 * \param mtime time
 */
void printFormattedTime(time_t mtime);

/*
 * Parse from text to time
 * \param str time text
 * \returns time
 */
time_t strToTime(const char *str);

/*
 * Extract hour from time
 * \param mtime time
 * \returns hour
 */
int extractHour(time_t mtime);

/*
 * Extract day of month from time
 * \param mtime time
 * \returns day of month
 */
int extractDay(time_t mtime);

/*
 * Get current local time formatted to string
 * \returns formatted current time
 */
string getTimeOfSimulationStart();

} // namespace ns3

#endif