FROM debian:testing

WORKDIR /app

RUN apt-get -y update
RUN apt-get -y install bash vim gcc g++ gdb cmake default-jdk
RUN ln -s /usr/lib/jvm/default-java/include/jni.h /usr/include/
RUN ln -s /usr/lib/jvm/default-java/include/linux/jni_md.h /usr/include/
RUN ln -s /usr/lib/jvm/default-java/include/jvmti.h /usr/include/

CMD /bin/bash
