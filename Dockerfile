ARG IDF_IMAGE_TAG=release-v5.5
FROM espressif/idf:${IDF_IMAGE_TAG}

WORKDIR /project

CMD ["bash"]
