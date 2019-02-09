FROM envoyproxy/envoy:latest
COPY envoy.yaml /etc/envoy/envoy.yaml
COPY scripts/run-envoy.sh /run-envoy.sh
ARG host
ARG port
ENV HOST=${host}
ENV PORT=${port}
ENTRYPOINT ./run-envoy.sh
