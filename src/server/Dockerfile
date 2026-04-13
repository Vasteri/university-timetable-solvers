FROM python:3.12.12-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    coinor-cbc \     
    && rm -rf /var/lib/apt/lists/*

#glpk-utils \

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

ENV PYTHONUNBUFFERED=1
EXPOSE 8000
COPY . .
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]