# Embedded AI Tensor Manager (TinyML Implementation)

Bu proje, Arduino ve ESP32 gibi hafızası (RAM) kısıtlı olan gömülü sistemlerde yapay zeka modellerini çalıştırmak için geliştirdiğim C kütüphanesidir.

## 🎯 Projenin Amacı
Normalde `float` (32-bit) dizileri mikrodenetleyicilerin belleğini hemen dolduruyor. Bu ödevde, **"Agentic Coding"** yöntemini kullanarak (Yapay zeka desteğiyle), verileri dinamik olarak yöneten bir yapı tasarladım.

Temel amacım şuydu:
1.  **Float32:** Hassas işlemler gerektiğinde kullanmak.
2.  **Int8 (Quantized):** Bellekten tasarruf etmek gerektiğinde veriyi sıkıştırmak (%75 Tasarruf).

## 🛠 Neler Kullandım?
* **Dil:** C Programlama Dili (Gömülü sistemlere uygun)
* **IDE:** Dev-C++
* **Yöntem:** Agentic Coding (Gemini 2.0 modelini teknik asistan olarak kullandım)
* **Hedef Donanım:** Arduino, ESP32, ARM Cortex serisi

## 🚀 Kodun Teknik Özellikleri
Kodun içinde şunları uyguladım:

1.  **MicroBuffer Yapısı:** Tensör verilerini yöneten ana struct yapısı.
2.  **Union Kullanımı:** Float ve Int8 verileri için ayrı ayrı yer ayırmak yerine `Union` kullanarak aynı bellek adresini paylaştırdım. Bu sayede RAM kullanımı düştü.
3.  **Quantization (Sıkıştırma):** Asimetrik quantization formülü ile sayıları `scale` ve `zero_point` kullanarak 8-bit tamsayılara dönüştürdüm.
4.  **Güvenli Bellek (Calloc):** `malloc` yerine `calloc` kullandım. Böylece bellekte çöp değerler (garbage value) kalmasını engelledim.

## 💻 Nasıl Çalıştırılır?
Kodu Dev-C++ veya herhangi bir C derleyicisi (GCC) ile derleyebilirsiniz.
```bash
gcc main.c -o tensor_app
./tensor_app
