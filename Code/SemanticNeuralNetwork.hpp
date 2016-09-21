#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "Util.hpp"
using namespace std;

class SemanticNeuralNetwork {
struct VocabNode {
string word;
int index;
int count;
int nodeCode;
vector<int> codeArray;
VocabNode *parent;  // for building huffman tree
VocabNode() {}
VocabNode(string word, int index, int count) {
  this->word = word;
  this->index = index;
  this->count = count;
  nodeCode = 0;
  parent = NULL;
}
VocabNode(int count) {
  this->word = "";
  this->index = -1;
  this->count = count;
  nodeCode = 0;
  parent = NULL;
}
};

struct VocabNodeComparator {
bool operator()(VocabNode *arg1, VocabNode *arg2) {
  return arg1->count > arg2->count;
}
};

enum DataType { TEXT, GENOME };

public:
DataType dataType = GENOME;

const int maxInnerIter = 5;
const int maxIteration = 50;
int hlsize = 256;  // hidden layer size
const int shiftSize = 3;
const int threadNum = 30;
const float alpha = 0.1;
const float beta = 0.05;  // for monument

int kmerSize = 5;

int vocabSize = 0;
int wordCount = 0;
int trainingWordCount = 0;
int trainingSentenceCount = 0;

map<string, VocabNode *> vocabMap;
// vocabulary map, word -> index,count
vector<VocabNode *> vocabVec;
vector<vector<int>> wordIndexInSentence;
float **WIH;
// input -> hidden weight  | vocabSize ＊ hlsize
// WIH[k][i] : weight of kth Input word to ith Hidden unit
float ***WHO;
// hidden -> output weight | hlsize * vocabSize
// WHO[threadIndex][i][j] weight of ith Hidden unit to jth Output unit
float **hidden;    // hidden[threadIndex][hlsize]
float **WIHe;      // WIHe[threadIndex][hlsize]
float **WIHe_pre;  // monument

void InitGenomeTraining(int kmerSize, int hidLayerSize) {
srand(time(0));
dataType = GENOME;
this->kmerSize = kmerSize;
this->hlsize = hidLayerSize;
}

void InitTextTraining() {
srand(time(0));
dataType = TEXT;
}

void CreateHuffmanTree() {
cout << "Create Huffman Tree" << endl;
map<string, VocabNode *>::iterator iter;
priority_queue<VocabNode *, vector<VocabNode *>, VocabNodeComparator> pq;
for (iter = vocabMap.begin(); iter != vocabMap.end(); iter++) {
  pq.push(iter->second);
}
while (pq.size() > 1) {
  VocabNode *rightNode = pq.top();
  pq.pop();
  VocabNode *leftNode = pq.top();
  pq.pop();
  VocabNode *parent = new VocabNode(leftNode->count + rightNode->count);
  leftNode->nodeCode = 1;
  rightNode->nodeCode = 0;
  leftNode->parent = parent;
  rightNode->parent = parent;
  pq.push(parent);
}

for (iter = vocabMap.begin(); iter != vocabMap.end(); iter++) {
  VocabNode *vn = iter->second;
  // cout << iter->first << vn->count << "| node code:" ;
  while (vn->parent != NULL) {
    iter->second->codeArray.push_back(vn->nodeCode);
    // cout << vn->nodeCode;
    vn = vn->parent;
  }
  // cout << endl;
}
// cout << " ------------- "<<endl;
// for (size_t i = 0; i < vocabVec.size(); i++) {
// 	cout << vocabVec[i]->word << " count: "
//      << vocabVec[i]->count << " Index: "
//      << vocabVec[i]->index <<" | code: ";
// 	for (size_t j = 0; j < vocabVec[i]->codeArray.size();
// j++) {
// 		cout << vocabVec[i]->codeArray[j];
// 	}
// 	cout << endl;
// }
cout << "Create Huffman Tree complete" << endl;
}

void InitArray(int vocabSize) {
WIH = new float *[vocabSize];
for (size_t i = 0; i < vocabSize; i++) {
  WIH[i] = new float[hlsize];
  for (size_t j = 0; j < hlsize; j++) {
    WIH[i][j] = UT_Math::RandFloat(-0.05, 0.05);
  }
}
WHO = new float **[threadNum];
for (int tindex = 0; tindex < threadNum; tindex++) {
  WHO[tindex] = new float *[hlsize];
  for (size_t i = 0; i < hlsize; i++) {
    WHO[tindex][i] = new float[vocabSize];
    for (size_t j = 0; j < vocabSize; j++) {
      WHO[tindex][i][j] = UT_Math::RandFloat(-0.05, 0.05);
    }
  }
}
hidden = new float *[threadNum];
WIHe = new float *[threadNum];
WIHe_pre = new float *[threadNum];
for (int i = 0; i < threadNum; i++) {
  hidden[i] = new float[hlsize];
  WIHe[i] = new float[hlsize];
  WIHe_pre[i] = new float[hlsize];
}
}

void InitTrainingData(const vector<vector<string>> wordsSentences) {
cout << "start init" << endl;
this->dataType = dataType;
for (int i = 0; i < wordsSentences.size(); i++) {
  vector<int> wordIndexArray;
  for (int j = 0; j < wordsSentences[i].size(); j++) {
    string word = wordsSentences[i][j];
    if (vocabMap.find(word) == vocabMap.end()) {
      VocabNode *vn = new VocabNode(word, vocabSize, 1);
      vocabMap[word] = vn;
      vocabVec.push_back(vn);
      vocabSize++;
    } else {
      vocabMap[word]->count++;
    }
    wordCount++;
    wordIndexArray.push_back(vocabMap[word]->index);
  }
  wordIndexInSentence.push_back(wordIndexArray);
}
CreateHuffmanTree();
InitArray(vocabSize);
cout << "Init Complete" << endl;
cout << "Vocab|kmer count:" << vocabSize << endl;
cout << "Word|kmer count:" << wordCount << endl;
cout << "sentence|gene count:" << wordIndexInSentence.size() << endl;
}

void InitTrainingData(const vector<string> &sentenceArray) {
cout << "start init" << endl;
this->dataType = dataType;
trainingWordCount = 0;
vocabSize = 0;
wordCount = 0;
for (int i = 0; i < sentenceArray.size(); i++) {
  string sentence = sentenceArray[i];
  // cout << sentenceArray[i] << endl;
  vector<string> wordArray;
  UT_String::split(UT_String::trim(sentence), ' ', wordArray);
  if (wordArray.size() < 3) continue;
  vector<int> wordIndexArray;
  for (int j = 0; j < wordArray.size(); j++) {
    // cout << wordArray[j] << "|";
    string word = wordArray[j];
    if (vocabMap.find(word) == vocabMap.end()) {
      VocabNode *vn = new VocabNode(word, vocabSize, 1);
      vocabMap[word] = vn;
      vocabVec.push_back(vn);
      vocabSize++;
    } else {
      vocabMap[word]->count++;
    }
    wordCount++;
    wordIndexArray.push_back(vocabMap[word]->index);
  }
  wordIndexInSentence.push_back(wordIndexArray);
}
CreateHuffmanTree();
InitArray(vocabSize);
cout << "Init Complete" << endl;
cout << "Vocab count:" << vocabSize << endl;
cout << "Word count:" << wordCount << endl;
}

void TrainEach(int *inputWordsIndex, int inputSize, int outputWordIndex,
             int threadIndex) {
int iterCount = 0;
memset(WIHe_pre[threadIndex], 0, sizeof(WIHe_pre[0][0]) * hlsize);
int outputLayerSize = vocabVec[outputWordIndex]->codeArray.size();
while (1) {
  memset(hidden[threadIndex], 0, sizeof(hidden[0][0]) * hlsize);
  memset(WIHe[threadIndex], 0, sizeof(WIHe[0][0]) * hlsize);
  // input -> hidden
  for (int k = 0; k < inputSize; k++) {
    for (int i = 0; i < hlsize; i++) {
      hidden[threadIndex][i] += (WIH[inputWordsIndex[k]][i]);
    }
  }
  for (int i = 0; i < hlsize; i++) {
    hidden[threadIndex][i] /= (float)inputSize;
  }
  float error = 0;
  for (int j = 0; j < outputLayerSize; j++) {
    float e = 0;
    float outputJ = 0;
    float target = vocabVec[outputWordIndex]->codeArray[j];
    for (int i = 0; i < hlsize; i++) {
      outputJ += hidden[threadIndex][i] * WHO[threadIndex][i][j];
    }
    outputJ = UT_Math::sigmoid(outputJ);
    e = alpha * (target - outputJ) * outputJ * (1 - outputJ);
    error = error + abs((target - outputJ));
    // for learning weight of input to hidden
    for (int i = 0; i < hlsize; i++) {
      WIHe[threadIndex][i] += e * WHO[threadIndex][i][j];
    }
    // learn weight of hidden to output layer
    for (int i = 0; i < hlsize; i++) {
      WHO[threadIndex][i][j] += e * hidden[threadIndex][i];
    }
  }
  // learn weight of input to hidden
  for (int k = 0; k < inputSize; k++) {
    for (int i = 0; i < hlsize; i++) {
      WIH[inputWordsIndex[k]][i] +=
          (WIHe[threadIndex][i]) + beta * (WIHe_pre[threadIndex][i]);
      WIHe_pre[threadIndex][i] = WIHe[threadIndex][i];
    }
  }
  iterCount++;
  if ((error / (outputLayerSize)) < 0.005 || iterCount > maxInnerIter) {
    // cout<< "iter count: " << iterCount << endl;
    break;
  }
}
}

void TrainingSentence(int sentenceIndex, int threadIndex) {
for (int i = 0; i < wordIndexInSentence[sentenceIndex].size(); i++) {
  int outputIndex = wordIndexInSentence[sentenceIndex][i];
  // escape from rare words
  if (vocabVec[outputIndex]->count < 2) continue;
  int inputArray[20];
  int inputCount = 0;
  int localShift = 0;
  for (int j = 1;; j++) {
    if (localShift >= shiftSize) {
      break;
    }
    if (i - j < 0 && i + j >= wordIndexInSentence[sentenceIndex].size()) {
      break;
    }
    bool isAdded = false;
    int inputIndex = 0;
    if (i - j > 0) {
      inputIndex = wordIndexInSentence[sentenceIndex][i - j];
      inputArray[inputCount++] = inputIndex;
      isAdded = true;
    }
    if (i + j < wordIndexInSentence[sentenceIndex].size()) {
      inputIndex = wordIndexInSentence[sentenceIndex][i + j];
      inputArray[inputCount++] = inputIndex;
      isAdded = true;
    }
    if (isAdded) {
      localShift++;
    }
  }

  if (inputCount == 0) continue;
  TrainEach(inputArray, inputCount, outputIndex, threadIndex);
  trainingWordCount++;
}
}

void TrainingSentence_Gene(int sentenceIndex, int threadIndex) {
for (int i = 0; i < wordIndexInSentence[sentenceIndex].size(); i++) {
  int outputIndex = wordIndexInSentence[sentenceIndex][i];
  int inputArray[20];
  int inputCount = 0;
  int localShift = 0;
  // int localKmerSize = kmerSize;  // kmer input not
  // overlap
  int localKmerSize = 1;  // kmer input overlap
  for (int j = localKmerSize;;
       j += localKmerSize) {  // kmer input not overlap
    if (localShift >= shiftSize) {
      break;
    }
    if (i - j < 0 && i + j >= wordIndexInSentence[sentenceIndex].size()) {
      break;
    }
    bool isAdded = false;
    int inputIndex = 0;
    // left kmer
    if (i - j > 0) {
      inputIndex = wordIndexInSentence[sentenceIndex][i - j];
      inputArray[inputCount++] = inputIndex;
      isAdded = true;
    }

    // right kmer
    if (i + j < wordIndexInSentence[sentenceIndex].size()) {
      inputIndex = wordIndexInSentence[sentenceIndex][i + j];
      inputArray[inputCount++] = inputIndex;
      isAdded = true;
    }
    if (isAdded) {
      localShift++;
    }
  }

  if (inputCount == 0) continue;
  TrainEach(inputArray, inputCount, outputIndex, threadIndex);
  trainingWordCount++;
}
}

void TrainingThread(int threadIndex) {
int eachTrainingSize = wordIndexInSentence.size() / threadNum;
int beginIndex = threadIndex * eachTrainingSize;
int endIndex = (threadIndex + 1) * eachTrainingSize;
if (threadIndex == threadNum - 1) {
  endIndex += wordIndexInSentence.size() % threadNum;
}
for (int i = 0; i < maxIteration; i++) {
  for (int i = beginIndex; i < endIndex; i++) {
    if (dataType == TEXT) {
      TrainingSentence(i, threadIndex);
    } else if (dataType == GENOME) {
      TrainingSentence_Gene(i, threadIndex);
    }
    trainingSentenceCount++;
    printf("\rProgress: %.3f%%", (float)trainingSentenceCount /
                                     (float)wordIndexInSentence.size() /
                                     maxIteration * 100.0);
    fflush(stdout);
  }
}
}

void Train() {
cout << "start training" << endl;
if (dataType == TEXT) {
  cout << "training Text data" << endl;
} else if (dataType == GENOME) {
  cout << "training Genome data" << endl;
}
vector<thread *> threadPool;
for (int i = 0; i < threadNum; i++) {
  thread *t = new thread(&SemanticNeuralNetwork::TrainingThread, this, i);
  threadPool.push_back(t);
}
for (int i = 0; i < threadNum; i++) {
  threadPool[i]->join();
}
for (int i = 0; i < threadNum; i++) {
  delete threadPool[i];
}
}

void Save(string path) {
cout << "\nSaving" << endl;
ofstream ofile(path);
map<string, VocabNode *>::iterator iter;
for (iter = vocabMap.begin(); iter != vocabMap.end(); iter++) {
  ofile << iter->first << endl;
  int wordIndex = iter->second->index;
  for (int i = 0; i < hlsize; i++) {
    ofile << WIH[wordIndex][i] << " ";
  }
  ofile << endl;
}
ofile.close();
cout << "Save completed\n" << endl;
}

void TestVocabMap() { CreateHuffmanTree(); }
};
