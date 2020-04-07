#ifndef BITVECTORRANK_H_
#define BITVECTORRANK_H_

#include <assert.h>
#include <vector>
#include "BitVector.hpp"
#include "popcount.h"
using namespace std;

class BitVectorRank:public BitVector {
public:
    BitVectorRank():basic_block_size_(256U), rank_lut_(nullptr) {};
    BitVectorRank(vector<uint64_t>& rankVector);
    ~BitVectorRank() {delete[] bits_; delete[] rank_lut_;}
    position_t rank(position_t pos) const;
    void serialize(char*& dst) const;
    //BitVectorRank* deSerialize(char*& src);//读入模块
	void deSerialize(char*& src);//读入模块
    position_t getSerializedSize() const; //Total Space
	void showRankLut();
private:
    position_t basic_block_size_ = Rank_Block_Size;
	position_t rank_lut_size = 0; 
    position_t* rank_lut_ = nullptr;
private:
    void initRankLut();
    void prefetch(position_t pos) const; 
};

BitVectorRank::BitVectorRank(vector<uint64_t>& rankVector):BitVector(rankVector)
{
	initRankLut();
	cout << "RankLut = " <<rank_lut_size<<endl;//-------------
}

	// | num_bits_(32) | num_words(32) | rank_lut_size(32)|bits_(64)| rank_lut_(32)
void BitVectorRank::serialize(char*& dst) const {
	memcpy(dst, &num_bits_, sizeof(num_bits_));
	dst += sizeof(num_bits_); 
	memcpy(dst, &num_words_, sizeof(num_words_)); 
	dst += sizeof(num_words_); 
	memcpy(dst, &rank_lut_size, sizeof(rank_lut_size)); 
	dst += sizeof(rank_lut_size);
	memcpy(dst, bits_, num_words_*sizeof(word_t)); 
	dst += num_words_*sizeof(word_t);
	memcpy(dst, rank_lut_, rank_lut_size*sizeof(position_t));
	dst += rank_lut_size*sizeof(position_t);
	align(dst);
}

void BitVectorRank::deSerialize(char*& src) {
	memcpy(&(num_bits_), src, sizeof(num_bits_)); 
    src += sizeof(num_bits_);
    memcpy(&(num_words_), src, sizeof(num_words_));
	src += sizeof(num_words_);
	memcpy(&(rank_lut_size),src,sizeof(position_t));
	src += sizeof(rank_lut_size);
	bits_ = const_cast<word_t*>(reinterpret_cast<const word_t*>(src));
	src += num_words_*sizeof(word_t);
	rank_lut_ = const_cast<position_t*>(reinterpret_cast<const position_t*>(src));
	src += rank_lut_size*sizeof(position_t);
	//memcpy(rank_lut_,src,rank_lut_size*sizeof(position_t));
	//src += rank_lut_size*sizeof(position_t);
	align(src);
}

position_t BitVectorRank::rank(position_t pos) const {
    assert(pos < num_bits_);
    position_t word_per_basic_block = basic_block_size_ / kWordSize; 
    position_t block_id = pos / basic_block_size_; 
    position_t offset = pos & (basic_block_size_ - 1); 
        return (rank_lut_[block_id] 
		+ popcountLinear(bits_, block_id * word_per_basic_block, offset + 1));
}

position_t BitVectorRank::getSerializedSize() const { 
	position_t size = sizeof(num_bits_) + sizeof(num_words_)+ num_words_*sizeof(word_t)
		+ sizeof(rank_lut_size) + rank_lut_size*sizeof(position_t);
	sizeAlign(size);//size8字节对齐
	return size;
}

void BitVectorRank::initRankLut() {
    position_t word_per_basic_block = basic_block_size_ / kWordSize;
    position_t num_blocks = num_words_ / basic_block_size_ + 1;
	rank_lut_ = new position_t[num_blocks];
	rank_lut_size = num_words_ / basic_block_size_ + 1;

    position_t cumu_rank = 0;
    for (position_t i = 0; i < num_blocks - 1; i++) {
        rank_lut_[i] = cumu_rank;
        cumu_rank += popcountLinear(bits_, i * word_per_basic_block, basic_block_size_);
    }
	rank_lut_[num_blocks - 1] = cumu_rank;
}

void BitVectorRank::prefetch(position_t pos) const {
	__builtin_prefetch(bits_ + (pos / kWordSize));  
	__builtin_prefetch(rank_lut_ + (pos / basic_block_size_)); 
}

void BitVectorRank::showRankLut() {
	cout << rank_lut_size <<endl;
	for(int i=0;i<rank_lut_size ;++i) {
		cout << rank_lut_[i]<<" ";
	}
	cout << endl;
}
#endif