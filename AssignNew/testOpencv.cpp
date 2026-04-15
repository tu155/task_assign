//// test_opencv.cpp - 测试文件
//#include <iostream>
//#include <opencv2/opencv.hpp>
//#include <opencv2/ml.hpp>
//
//int main() {
//    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
//
//    // 测试机器学习模块
//    cv::Ptr<cv::ml::DTrees> dtree = cv::ml::DTrees::create();
//    if (!dtree.empty()) {
//        std::cout << "✓ OpenCV ML module is working!" << std::endl;
//
//        // 简单测试
//        dtree->setMaxDepth(5);
//        std::cout << "✓ Decision Tree created with max depth 5" << std::endl;
//    }
//
//    return 0;
//}