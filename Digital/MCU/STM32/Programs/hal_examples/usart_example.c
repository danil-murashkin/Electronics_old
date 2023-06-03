/* USER CODE BEGIN 0 */
uint8_t byte[1];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART3)
	{
		HAL_UART_Transmit(&huart3, byte, 1, 0); //no timeout
		HAL_UART_Receive_IT(&huart3, byte, 1);
	}
}
/* USER CODE END 0 */

//int main (void)
	/* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart3, byte, 1); //init
	/* USER CODE END 2 */
//while(1)