#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <dirent.h>
#include "Word.h"
#include "LinkedList.h"

#define DEBUG 0
#define MAX_FILES 50

const char *stop_words[] = {
    "i", "me", "my", "myself", "we", "our", "ours", "ourselves", "you", "your", "yours", "yourself", "yourselves",
    "he", "him", "his", "himself", "she", "her", "hers", "herself", "it", "its", "itself", "they", "them", "their", "theirs", "themselves",
    "what", "which", "who", "whom", "this", "that", "these", "those", "am", "is", "are", "was", "were", "be", "been", "being",
    "have", "has", "had", "having", "do", "does", "did", "doing", "a", "an", "the", "and", "but", "if", "or", "because", "as", "until", "while",
    "of", "at", "by", "for", "with", "about", "against", "between", "into", "through", "during", "before", "after", "above", "below", "to",
    "from", "up", "down", "in", "out", "on", "off", "over", "under", "again", "further", "then", "once", "here", "there", "when", "where", "why", "how",
    "all", "any", "both", "each", "few", "more", "most", "other", "some", "such", "no", "nor", "not", "only", "own", "same", "so", "than", "too", "very",
    "s", "t", "can", "will", "just", "don", "should", "now"};
#define total_stop_words (sizeof(stop_words) / sizeof(const char *))

bool is_stop_word(char *word_text)
{
    for (int i = 0; i < total_stop_words; i++)
    {
        if (strcmp(word_text, stop_words[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

const char punctutations[] = {'.', ',', ';', '\'', '\"', ':', '-', '_', '(', ')', '[', ']', '{', '}', '*', '!', '$', '?'};
#define total_punctuations (sizeof(punctutations) / sizeof(const char))

char *pre_process_word(char *word_text)
{
    int j = 0;
    for (int i = 0; word_text[i]; i++)
    {
        bool is_punctuation = false;
        for (int p = 0; p < total_punctuations; p++)
        {
            if (word_text[i] == punctutations[p])
            {
                is_punctuation = true;
                break;
            }
        }

        if (!is_punctuation)
        {
            word_text[j] = tolower(word_text[i]);
            j++;
        }
    }
    word_text[j] = '\0';

    if (j == 0 || is_stop_word(word_text)) // check for empty word or stop word
    {
        return "";
    }

    return word_text;
}

// Find word in given list
Word *find_word(LinkedList *word_list, char *word_text)
{
    if (word_list->head == NULL)
    {
        return NULL;
    }

    ll_Node *current = word_list->head;

    while (strcmp(((Word *)current->data)->word_text, word_text) != 0)
    {
        if (current->next == NULL)
        {
            return NULL;
        }
        else
        {
            current = current->next;
        }
    }

    return ((Word *)current->data);
}

// Print word list
void print_word_list(LinkedList *word_list)
{
    ll_Node *current = word_list->head;

    while (current != NULL)
    {
        char *word_text = ((Word *)current->data)->word_text;
        int frequency = ((Word *)current->data)->frequency;
        printf("Word: %s %d\n", word_text, frequency);
        current = current->next;
    }
}

// Cosine similarity
double cosine_similarity(double vector1[], double vector2[], int size)
{
    double dot_product = 0;
    double mod1 = 0;
    double mod2 = 0;

    for (int i = 0; i < size; i++)
    {
        dot_product += vector1[i] * vector2[i];
        mod1 += vector1[i] * vector1[i];
        mod2 += vector2[i] * vector2[i];
    }

#if DEBUG
    printf("%f %f %f\n", dot_product, mod1, mod2);
#endif

    return (dot_product) / (sqrt(mod1 * mod2));
}

int check_using_cosine_similarity(char *test_file, char *corpus)
{
    FILE *test_file_ptr = fopen(test_file, "r");
    if (test_file_ptr == NULL)
    {
        printf("Unable to read test file\n");
        return 0;
    }

    LinkedList *test_file_words = ll_new_list(sizeof(Word));
    LinkedList *compare_file_words_array[MAX_FILES];

    char word_text[MAX_WORD_SIZE];
    while (fscanf(test_file_ptr, " %1023s", word_text) == 1)
    {
        char *processed_word_text = pre_process_word(word_text);
        if (strcmp(processed_word_text, "") == 0)
        {
            continue;
        }

        Word *find = find_word(test_file_words, processed_word_text);
        if (find == NULL)
        {
            Word *word = new_word(processed_word_text);
            ll_append(test_file_words, (void *)word);
        }
        else
        {
            find->frequency++;
        }
    }
    fclose(test_file_ptr);

    struct dirent *de; // Pointer for directory entry
    DIR *dr = opendir(corpus);

    if (dr == NULL)
    {
        printf("Could not open corpus folder");
        return 0;
    }

    int file_count = 0;
    while ((de = readdir(dr)) != NULL)
    {
        char *compare_file_name = de->d_name;
        if (strcmp(compare_file_name, ".") == 0 || strcmp(compare_file_name, "..") == 0)
        {
            continue;
        }

        char *compare_file = malloc(strlen(corpus) + strlen(compare_file_name) + 1); // +1 for the null-terminator
        strcpy(compare_file, corpus);
        strcat(compare_file, compare_file_name);

        FILE *compare_file_ptr = fopen(compare_file, "r");
        if (compare_file_ptr == NULL)
        {
            printf("Unable to read compare file: %s\n", compare_file);
            continue;
        }

        compare_file_words_array[file_count] = ll_new_list(sizeof(Word));
        LinkedList *combined_words = ll_new_list(sizeof(Word));

        ll_Node *curr_test_file_word = test_file_words->head;
        while (curr_test_file_word != NULL)
        {
            char *test_file_word_text = ((Word *)curr_test_file_word->data)->word_text;
            Word *find_in_combined_words = find_word(combined_words, test_file_word_text);
            if (find_in_combined_words == NULL)
            {
                Word *word = new_word(test_file_word_text);
                ll_append(combined_words, (void *)word);
            }
            else
            {
                find_in_combined_words->frequency++;
            }
            curr_test_file_word = curr_test_file_word->next;
        }

        while (fscanf(compare_file_ptr, " %1023s", word_text) == 1)
        {
            char *processed_word_text = pre_process_word(word_text);
            if (strcmp(processed_word_text, "") == 0)
            {
                continue;
            }

            Word *find_in_compare_file_words = find_word(compare_file_words_array[file_count], processed_word_text);
            if (find_in_compare_file_words == NULL)
            {
                Word *word = new_word(processed_word_text);
                ll_append(compare_file_words_array[file_count], (void *)word);
            }
            else
            {
                find_in_compare_file_words->frequency++;
            }

            Word *find_in_combined_words = find_word(combined_words, processed_word_text);
            if (find_in_combined_words == NULL)
            {
                Word *word = new_word(processed_word_text);
                ll_append(combined_words, (void *)word);
            }
            else
            {
                find_in_combined_words->frequency++;
            }
        }
        fclose(compare_file_ptr);
        free(compare_file);

#if DEBUG
        printf("list compare file %d\n", file_count);
        print_word_list(compare_file_words_array[file_count]);

        printf("list all\n");
        print_word_list(combined_words);
#endif

        int total_combined_words = combined_words->length;
        double word_vector_1[total_combined_words];
        double word_vector_2[total_combined_words];

        ll_Node *curr_combined_word = combined_words->head;
        int non_stop_words = 0;
        while (curr_combined_word != NULL)
        {
            char *curr_combined_word_text = ((Word *)curr_combined_word->data)->word_text;

            Word *word1 = find_word(test_file_words, curr_combined_word_text);
            if (word1 != NULL)
            {
                word_vector_1[non_stop_words] = (double)(word1->frequency) / (test_file_words->length);
            }
            else
            {
                word_vector_1[non_stop_words] = 0;
            }

            Word *word2 = find_word(compare_file_words_array[file_count], curr_combined_word_text);
            if (word2 != NULL)
            {
                word_vector_2[non_stop_words] = (double)(word2->frequency) / (compare_file_words_array[file_count]->length);
            }
            else
            {
                word_vector_2[non_stop_words] = 0;
            }

            non_stop_words++;

            curr_combined_word = curr_combined_word->next;
        }

#if DEBUG
        printf("Word vector size: %d\n", non_stop_words);

        for (int i = 0; i < non_stop_words; i++)
        {
            printf("word vectors: %f %f\n", word_vector_1[i], word_vector_2[i]);
        }
#endif

        double similarity = cosine_similarity(word_vector_1, word_vector_2, non_stop_words);
        printf("%s %.2f\n", compare_file_name, similarity * 100.0);

        // Free combined words
        while (!ll_is_empty(combined_words))
        {
            ll_remove_head(combined_words);
        }
        free(combined_words);

        file_count++;
    }
    closedir(dr);

    for (int i = 0; i < file_count; i++)
    {
        // Free compare_file_words_array[i]
        while (!ll_is_empty(compare_file_words_array[i]))
        {
            ll_remove_head(compare_file_words_array[i]);
        }
        free(compare_file_words_array[i]);
    }

    // Free test file words
    while (!ll_is_empty(test_file_words))
    {
        ll_remove_head(test_file_words);
    }
    free(test_file_words);

    return 0;
}

int check_using_cosine_similarity_with_TF_IDF(char *test_file, char *corpus)
{
    FILE *test_file_ptr = fopen(test_file, "r");
    if (test_file_ptr == NULL)
    {
        printf("Unable to read test file\n");
        return 0;
    }

    LinkedList *test_file_words = ll_new_list(sizeof(Word));
    LinkedList *all_words = ll_new_list(sizeof(Word));
    LinkedList *compare_file_words_array[MAX_FILES];

    char word_text[MAX_WORD_SIZE];
    while (fscanf(test_file_ptr, " %1023s", word_text) == 1)
    {
        char *processed_word_text = pre_process_word(word_text);
        if (strcmp(processed_word_text, "") == 0)
        {
            continue;
        }

        Word *find = find_word(test_file_words, processed_word_text);
        if (find == NULL)
        {
            Word *word = new_word(processed_word_text);
            ll_append(test_file_words, (void *)word);
        }
        else
        {
            find->frequency++;
        }
    }
    fclose(test_file_ptr);

    struct dirent *de; // Pointer for directory entry
    DIR *dr = opendir(corpus);

    if (dr == NULL)
    {
        printf("Could not open corpus folder");
        return 0;
    }

    int file_count = 0;
    char file_names[MAX_FILES][MAX_WORD_SIZE];
    while ((de = readdir(dr)) != NULL)
    {
        char *compare_file_name = de->d_name;
        if (strcmp(compare_file_name, ".") == 0 || strcmp(compare_file_name, "..") == 0)
        {
            continue;
        }

        char *compare_file = malloc(strlen(corpus) + strlen(compare_file_name) + 1); // +1 for the null-terminator
        strcpy(compare_file, corpus);
        strcat(compare_file, compare_file_name);

        FILE *compare_file_ptr = fopen(compare_file, "r");
        if (compare_file_ptr == NULL)
        {
            printf("Unable to read compare file: %s\n", compare_file);
            continue;
        }

        compare_file_words_array[file_count] = ll_new_list(sizeof(Word));

        while (fscanf(compare_file_ptr, " %1023s", word_text) == 1)
        {
            char *processed_word_text = pre_process_word(word_text);
            if (strcmp(processed_word_text, "") == 0)
            {
                continue;
            }

            Word *find_in_compare_file_words = find_word(compare_file_words_array[file_count], processed_word_text);
            if (find_in_compare_file_words == NULL)
            {
                Word *word = new_word(processed_word_text);
                ll_append(compare_file_words_array[file_count], (void *)word);
            }
            else
            {
                find_in_compare_file_words->frequency++;
            }

            Word *find_in_all_words = find_word(all_words, processed_word_text);
            if (find_in_all_words == NULL)
            {
                Word *word = new_word(processed_word_text);
                ll_append(all_words, (void *)word);
            }
            else
            {
                find_in_all_words->frequency++;
            }
        }
        fclose(compare_file_ptr);
        free(compare_file);

        strcpy(file_names[file_count], compare_file_name);
        file_count++;
    }
    closedir(dr);

    ll_Node *curr_word = all_words->head;
    while (curr_word != NULL)
    {
        char *curr_word_text = ((Word *)curr_word->data)->word_text;

        int word_count = 0;
        for (int i = 0; i < file_count; i++)
        {
            Word *find_word_in_file = find_word(compare_file_words_array[i], curr_word_text);
            if (find_word_in_file != NULL)
            {
                word_count++;
            }
        }
        ((Word *)curr_word->data)->idf = 1 + log((double)file_count / word_count);

#if DEBUG
        printf("word idf %s %f\n", curr_word_text, ((Word *)curr_word->data)->idf);
#endif

        curr_word = curr_word->next;
    }

    for (int i = 0; i < file_count; i++)
    {
        LinkedList *combined_words = ll_new_list(sizeof(Word));

        ll_Node *curr_test_file_word = test_file_words->head;
        while (curr_test_file_word != NULL)
        {
            char *test_file_word_text = ((Word *)curr_test_file_word->data)->word_text;
            Word *find_in_combined_words = find_word(combined_words, test_file_word_text);
            if (find_in_combined_words == NULL)
            {
                Word *word = new_word(test_file_word_text);
                ll_append(combined_words, (void *)word);
            }
            else
            {
                find_in_combined_words->frequency++;
            }
            curr_test_file_word = curr_test_file_word->next;
        }

        ll_Node *curr_file_word = compare_file_words_array[i]->head;
        while (curr_file_word != NULL)
        {
            char *curr_file_word_text = ((Word *)curr_file_word->data)->word_text;
            Word *find_in_combined_words = find_word(combined_words, curr_file_word_text);
            if (find_in_combined_words == NULL)
            {
                Word *word = new_word(curr_file_word_text);
                ll_append(combined_words, (void *)word);
            }
            else
            {
                find_in_combined_words->frequency++;
            }
            curr_file_word = curr_file_word->next;
        }

        int total_combined_words = combined_words->length;
        double word_vector_1[total_combined_words];
        double word_vector_2[total_combined_words];

        ll_Node *curr_combined_word = combined_words->head;
        int non_stop_words = 0;
        while (curr_combined_word != NULL)
        {
            char *curr_combined_word_text = ((Word *)curr_combined_word->data)->word_text;

            Word *word0 = find_word(all_words, curr_combined_word_text);
            if (word0 == NULL)
            {
                printf("Error in all words\n");
                return 0;
            }
            double idf = word0->idf;

            Word *word1 = find_word(test_file_words, curr_combined_word_text);
            if (word1 != NULL)
            {
                word_vector_1[non_stop_words] = (double)((double)idf * word1->frequency) / (test_file_words->length);
            }
            else
            {
                word_vector_1[non_stop_words] = 0;
            }

            Word *word2 = find_word(compare_file_words_array[i], curr_combined_word_text);
            if (word2 != NULL)
            {
                word_vector_2[non_stop_words] = (double)(double)idf * (word2->frequency) / (compare_file_words_array[i]->length);
            }
            else
            {
                word_vector_2[non_stop_words] = 0;
            }

            non_stop_words++;

            curr_combined_word = curr_combined_word->next;
        }

#if DEBUG
        printf("Word vector size: %d\n", non_stop_words);

        for (int i = 0; i < non_stop_words; i++)
        {
            printf("word vectors: %f %f\n", word_vector_1[i], word_vector_2[i]);
        }
#endif

        double similarity = cosine_similarity(word_vector_1, word_vector_2, non_stop_words);
        printf("%s %.2f\n", file_names[i], similarity * 100.0);

        // Free combined words
        while (!ll_is_empty(combined_words))
        {
            ll_remove_head(combined_words);
        }
        free(combined_words);

        // Free compare_file_words_array[i]
        while (!ll_is_empty(compare_file_words_array[i]))
        {
            ll_remove_head(compare_file_words_array[i]);
        }
        free(compare_file_words_array[i]);
    }

    // Free test file words
    while (!ll_is_empty(test_file_words))
    {
        ll_remove_head(test_file_words);
    }
    free(test_file_words);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 4)
    {
        printf("Enter all argumentes\n");
        return 0;
    }

    char *test_file = argv[1];
    char *corpus = argv[2];
    bool use_tf_idf = argc >= 4 && strcmp(argv[3], "same-theme") == 0 ? true : false;

    if (use_tf_idf)
    {
        check_using_cosine_similarity_with_TF_IDF(test_file, corpus);
    }
    else
    {
        check_using_cosine_similarity(test_file, corpus);
    }

    return 0;
}