#ifndef BME280_HELPER_H
#define BME280_HELPER_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif


#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bme280.h"


// copied from https://github.com/boschsensortec/BME280_SensorAPI/blob/master/examples/common/common.h


bool bme280_init_force_i2c(struct bme280_dev *dev, i2c_inst_t *i2c, uint baudrate);
void bme280_init(struct bme280_dev *dev, i2c_inst_t *i2c, uint baudrate);

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 * @param[in] reg_addr       : Register address from which data is read.
 * @param[out] reg_data      : Pointer to data buffer where read data is stored.
 * @param[in] length         : Number of bytes of data to be read.
 * @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                             for interface related call backs.
 *
 *  @return Status of execution
 *
 *  @retval BME280_INTF_RET_SUCCESS -> Success.
 *  @retval != BME280_INTF_RET_SUCCESS -> Failure.
 *
 */
int8_t bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 * @param[in] reg_addr      : Register address to which the data is written.
 * @param[in] reg_data      : Pointer to data buffer in which data to be written
 *                            is stored.
 * @param[in] length        : Number of bytes of data to be written.
 * @param[in, out] intf_ptr : Void pointer that can enable the linking of descriptors
 *                            for interface related call backs
 *
 *  @return Status of execution
 *
 *  @retval BME280_INTF_RET_SUCCESS -> Success.
 *  @retval != BME280_INTF_RET_SUCCESS -> Failure.
 *
 */
int8_t bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr);

/*!
 *  @brief This function provides the delay for required time (Microsecond) as per the input provided in some of the
 *  APIs.
 *
 *  @param[in] period_us  : The required wait time in microsecond.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return void.
 */
void bme280_delay_us(uint32_t period_us, void *intf_ptr);

/*!
 *  @brief This API is used to print the execution status.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void bme280_error_codes_print_result(const char api_name[], int8_t rslt);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* BME280_HELPER_H */
