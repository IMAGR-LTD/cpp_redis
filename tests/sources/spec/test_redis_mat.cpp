// The MIT License (MIT)
//
// Copyright (c) 2015-2017 Simon Ninon <simon.ninon@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <thread>

#include <cpp_redis/core/client.hpp>
#include <cpp_redis/core/reply.hpp>
#include <cpp_redis/misc/error.hpp>

#include <gtest/gtest.h>

#include "opencv2/opencv.hpp"
#include <sstream>

#include <chrono>

using namespace cv;
using namespace std::chrono;
using namespace std;
using namespace cpp_redis;


#define IMAGE_HEIGHT       600
#define IMAGE_WIDTH        850

#include <iostream>

/// Serialize a cv::Mat to a stringstream
std::stringstream serialize(Mat input)
{
    // We will need to also serialize the width, height, type and size of the matrix
    int width = input.cols;
    int height = input.rows;
    int type = input.type();
    size_t size = input.total() * input.elemSize();

    // Initialize a stringstream and write the data
    std::stringstream ss;
    ss.write((char*)(&width), sizeof(int));
    ss.write((char*)(&height), sizeof(int));
    ss.write((char*)(&type), sizeof(int));
    ss.write((char*)(&size), sizeof(size_t));

    // Write the whole image data
    ss.write((char*)input.data, size);

    return ss;
}

// Deserialize a Mat from a stringstream
Mat deserialize(std::stringstream& input)
{
    // The data we need to deserialize
    int width = 0;
    int height = 0;
    int type = 0;
    size_t size = 0;

    // Read the width, height, type and size of the buffer
    input.read((char*)(&width), sizeof(int));
    input.read((char*)(&height), sizeof(int));
    input.read((char*)(&type), sizeof(int));
    input.read((char*)(&size), sizeof(size_t));

    // Allocate a buffer for the pixels
    char* data = new char[size];
    // Read the pixels from the stringstream
    input.read(data, size);

    // Construct the image (clone it so that it won't need our buffer anymore)
    Mat m = Mat(height, width, type, data).clone();

    // Delete our buffer
    delete[]data;

    // Return the matrix
    return m;
}

bool matIsEqual(const Mat mat1, const Mat mat2){
    // treat two empty mat as identical as well
    if (mat1.empty() && mat2.empty()) {
        return true;
    }
    // if dimensionality of two mat is not identical, these two mat is not identical
    if (mat1.cols != mat2.cols || mat1.rows != mat2.rows || mat1.dims != mat2.dims) {
        return false;
    }
    Mat diff;
    compare(mat1, mat2, diff, CMP_NE);
    int nz = countNonZero(diff);
    return nz==0;
}


TEST(RedisMatClient, TestMatFiles) {
  std::future<reply> getdata[1000];
  cpp_redis::client client;
  high_resolution_clock::time_point c_start;
  high_resolution_clock::time_point c_end;
  duration<double> time_span;
  duration<double> total_redis_time; 
  duration<double> total_opencv_time; 
  Mat img = imread("test.jpg", CV_LOAD_IMAGE_COLOR);
  Mat decodeImg;
  // randu(img, Scalar(0, 0, 0), Scalar(255, 255, 255));

  client.connect();

  client.send({"PING"}, [&](cpp_redis::reply& reply) {
    EXPECT_TRUE(reply.is_string());
    EXPECT_TRUE(reply.as_string() == "PONG");
  });
  client.sync_commit();


  for (int i = 0; i < 100; i++)
  {
    c_start = high_resolution_clock::now();
    std::string s = serialize(img).str();

    client.set("mat" + to_string(i), s);
    
    getdata[i] = client.get("mat" + to_string(i));
    
    c_end = high_resolution_clock::now();

    time_span = duration_cast<duration<double>>(c_end - c_start);

    total_redis_time += time_span;
  }

  client.sync_commit();

  std::stringstream ss;
  reply dataget = getdata[40].get();
  EXPECT_TRUE(dataget.is_string());
  std::string str = dataget.as_string();
  ss << str;
  decodeImg = deserialize(ss);
  // imwrite("decoded.jpg" , decodeImg);

  
  cout << "Redis Duration: " << total_redis_time.count() << endl;

  Mat readImg = imread("test.jpg", CV_LOAD_IMAGE_COLOR);

  for (int i = 0; i < 100; i++)
  {
    c_start = high_resolution_clock::now();
    
    // c_end = high_resolution_clock::now();

    // time_span = duration_cast<duration<double>>(c_end - c_start);
    // cout << "Read Duration: " << time_span.count() << endl;

    // c_start = high_resolution_clock::now();
    imwrite("imwrite" + to_string(i) + ".jpg", readImg);

    Mat readNewImg = imread("imwrite" + to_string(i) + ".jpg", CV_LOAD_IMAGE_COLOR);

    c_end = high_resolution_clock::now();

    time_span = duration_cast<duration<double>>(c_end - c_start);

    total_opencv_time += time_span;
  }
  // cout << "Write Duration: " << time_span.count() << endl;

  cout << "Opencv Duration: " << total_opencv_time.count() << endl;



  // client.send({"SET", "HELLO", "MultipleSend"}, [&](cpp_redis::reply& reply) {
  //   EXPECT_TRUE(reply.is_string());
  //   EXPECT_TRUE(reply.as_string() == "OK");
  // });
  // client.sync_commit();

  // client.send({"GET", "HELLO"}, [&](cpp_redis::reply& reply) {
  //   EXPECT_TRUE(reply.is_string());
  //   EXPECT_TRUE(reply.as_string() == "MultipleSend");
  // });
  // client.sync_commit();
}