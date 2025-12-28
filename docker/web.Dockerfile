FROM python:3.9-slim

WORKDIR /app

# Install dependencies
COPY web/requirements.txt /tmp/requirements.txt
RUN pip install -r /tmp/requirements.txt

# The application code will be mounted at runtime to /app
# ensuring access to the latest assets and code.

EXPOSE 8000
EXPOSE 8080
