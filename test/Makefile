# Makefile for OCR Mock Test
# PC上でのクイックテスト用

CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = ocr_mock_test

.PHONY: all clean run

all: $(TARGET)

$(TARGET): ocr_mock_test.c
	$(CC) $(CFLAGS) -o $(TARGET) ocr_mock_test.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe

test: run
	@echo ""
	@echo "✅ Test completed successfully!"
	@echo "🎯 Next: Integrate into STM32N6 project"
