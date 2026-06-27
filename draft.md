Create code for simplified billing of a mobile network operator.

We use the following rules for calculating call cost:
* Fixed connection fee of 0.33 that is added to any call cost.
* Minute fee is charged at the beginning of each minute, so if call duration is 1:03 two minutes cost should be paid
* Each subscriber has 30 minutes of free talking inside his operator network, valid for 30 days since the date when last credit was added.
* After free minutes expire, calls inside home operator network are charged 0.50 per minute.
* When calling numbers outside of home network, minute cost is 0.95
* On weekends, first five minutes of every call are free.

We define home network of the operator as a set of phone numbers starting with one of the given prefixes (050, 066, 095 and 099)

Create code calculating call cost, given date and time for its start and end, number called and subscriber account information.

Make code readable and easy to configure, maintain and modify.

Prepare demo application that allows testing the code with some example data.

Do not use any platform specific libraries or implementation-specific language features. Include any supplementary code, such as unit tests. Provide build scripts used for compilation and detailed instructions how to use them.