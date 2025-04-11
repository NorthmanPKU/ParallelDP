#include "include/tournament_tree.h"

/** 这个文件只是供我测试编译不报错使用 */

template class TournamentTree<int>;
template class TournamentTree<float>;
template class TournamentTree<double>;
template class TournamentTree<long>;
template class TournamentTree<char>;
template class TournamentTree<std::string>;

template class TournamentTree<int, std::greater<int>>;