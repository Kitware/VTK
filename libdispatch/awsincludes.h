/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */

/**
 * @author Dennis Heimbigner
 */

#include <aws/core/Aws.h> /* Needed for InitAPI, SDKOptions, Logging,
                             and ShutdownAPI */
#include <aws/s3/S3Client.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/Version.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
