#!/usr/bin/env bash

echo $HOST
echo $PORT
sed -i -e "s@REPLACE_WITH_HOST@$(echo $HOST | sed 's/\./\\./g')@g" /etc/envoy/envoy.yaml
sed -i -e "s@REPLACE_WITH_PORT@$PORT@g" /etc/envoy/envoy.yaml
/usr/local/bin/envoy -c /etc/envoy/envoy.yaml
