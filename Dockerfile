ARG IDF_IMAGE_TAG=release-v5.5
FROM espressif/idf:${IDF_IMAGE_TAG}

ARG IDF_TARGET=esp32s3
ENV IDF_TARGET=${IDF_TARGET}

WORKDIR /project
COPY . .

RUN export IDF_PATH_FORCE=1 && \
    . "$IDF_PATH/export.sh" && \
    idf.py set-target ${IDF_TARGET} && \
    idf.py build

CMD ["bash"]
