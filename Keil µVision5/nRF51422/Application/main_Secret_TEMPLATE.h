#ifndef MAIN_SECRET_H
#define MAIN_SECRET_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Network Keys:
 * If you are building a non-ANT+ product (i.e. an ANT product that does not
 * implement an ANT+ Device Profile) you may not use the ANT+ network key or the
 * ANT+ frequency (2457MHz). You may use any other frequency and either the
 * publicnetwork key or a private network key.
 * @see: https://www.thisisant.com/developer/ant-plus/ant-plus-basics/network-keys
*/
#error Become ANT-Adopter: https://www.thisisant.com/business/go-ant/levels-and-benefits
#define ANT_PUBLIC_NETWORK_KEY           {0x00, 0x00, 0x00, 0x0, 0x00, 0x00, 0x00, 0x00}                     /**< The default network key used. */


#endif /* MAIN_SECRET_H */
