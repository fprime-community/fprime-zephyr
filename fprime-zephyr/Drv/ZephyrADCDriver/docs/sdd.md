# Zephyr::ZephyrADCDriver

Zephyr ADC Driver for the fprime-zephyr OSAL layer providing access to the Zephyr ADC driver APIs. 
Currently only supports 12 bit resolution and single channel sampling.

## Configuration

ADC configuration is done through the Zephyr Device Tree. The ADC driver will use this configuration.

```dts
&adc1 {
	#address-cells = <1>;
	#size-cells = <0>;

	clocks = <&rcc STM32_CLOCK(AHB1, 5)>,
		 <&rcc STM32_SRC_CKPER ADC_SEL(2)>;
	clock-names = "adcx", "adc_ker";
	pinctrl-0 = <&adc1_inp0_pa0_c>;
	pinctrl-names = "default";
	st,adc-clock-source = "ASYNC";
	st,adc-prescaler = <4>;
	status = "okay";

	potentiometer: channel@0 {
		reg = <0>;
		zephyr,gain = "ADC_GAIN_1";
		zephyr,reference = "ADC_REF_INTERNAL";
		zephyr,vref-mv = <3300>;
		zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
		zephyr,resolution = <12>;
	};
};
```

```Cpp
#define FPRIME_ADC_NODE DT_PATH(zephyr_user)
static const struct adc_dt_spec adc_configuration_storage =
    ADC_DT_SPEC_GET_BY_NAME(FPRIME_ADC_NODE, potentiometer);
const struct adc_dt_spec* adc_configuration = &adc_configuration_storage;

void configureTopology(){
    adcDriver.configure(adc_configuration);
};
```
